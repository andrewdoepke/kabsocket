#include <iostream>
#include <string>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <random>
#include "base64.h"
#include <future>
#include <chrono>
#include <bitset>
#include <cmath>

using boost::asio::steady_timer;
using boost::asio::ip::tcp;

using namespace boost::asio;
namespace bs = boost::system;
using ip::tcp;
using std::string;
using std::cout;
using std::cin;
using std::endl;
using std::ios;
using std::ofstream;

const int PORT = 1234; //Simulated port for this program
const string HOST = "127.0.0.1";

namespace pt = boost::property_tree;

typedef std::vector<int> IntVec;

const char delim = '\x04';

//--------------Define Flags--------------//
// https://www.geeksforgeeks.org/tcp-flags/
//If changed, we need to also copy this to client code.
	const uint8_t IGN = 231; //ignore
	const uint8_t SYN = 0; //Synchronization
	const uint8_t ACK = 1; //Acknowledgement
	const uint8_t FIN = 2; //Finish
	const uint8_t RST = 3; //Reset
	const uint8_t PSH = 4; //Push
	const uint8_t URG = 5; //Urgent

	//For sliding window protocol. This will be separate from normal TCP flags.
	const uint8_t GBN = 6;
	const uint8_t SLR = 7;


	//Associate strings with the numbers
	std::string getConstStr(uint8_t num){
		switch(num){
			case 231:
				return "IGN";
				break;
			case 0:
				return "SYN";
				break;
			case 1:
				return "ACK";
				break;
			case 2:
				return "FIN";
				break;
			case 3:
				return "RST";
				break;
			case 4:
				return "PSH";
				break;
			case 5:
				return "URG";
				break;
			case 6:
				return "GBN";
				break;
			case 7:
				return "SLR";
				break;
			default:
				return "Empty or invalid..";
				break;
		}

		return "";
	}

//---------------------------------------//
//TCP Packet Header
//We can maybe just comment some of these out if they're not gonna be used.
//https://www.techrepublic.com/article/exploring-the-anatomy-of-a-data-packet/

struct tcp_header {
	uint16_t s_port = PORT; //sauce port. This is a default value rn but if we do multithreading, we'd have to select from a specified list of open ports.
	uint16_t d_port = PORT; //destination port. Standard in a packet but it's also good to use the same ports on both sides.
	uint32_t seq_num; //sequence number. This will be generated during runtime.
	uint32_t ack_num; //acknowledgement number.
	unsigned int offset; //Data Offset (entire size of header in bits % 32 bits)
	//uint8_t reserved = 0; //Reserved field. Doesn't do anything, just leave it as 0.
	uint8_t flag; //Packet flag. Use like: h.flag = ACK; or like if(h.flag == ACK) {...}
	uint16_t window; // Window size of sender's receive window.
	uint16_t checksum; //Checksum value
	//uint16_t urgent_ptr; //Pointer to urgent byte if sender wants it to be urgent. We could prolly implement this but idk.
	//string options; //TCP options
	//string upper_level_data; //Data for upper layer. Idk what this is for.
	//uint8_t sw_prot; //Type of sliding window protocol used. Can either be GBN or SR

	//to JSON function of a tcp_header
	std::string toJson(){
		pt::ptree pkt;
		pkt.put("s_port", s_port);
		pkt.put("d_port", d_port);
		pkt.put("seq_num", seq_num);
		pkt.put("ack_num", ack_num);
		pkt.put("offset", offset);
		pkt.put("flag", flag);
		pkt.put("window", window);
		pkt.put("checksum", checksum);

		std::stringstream ss;
		pt::json_parser::write_json(ss, pkt);

		return ss.str(); //return JSON Header
	}

	//to String function of a tcp_header
	std::string toString(){
		std::string s_port_s = "";
		std::string d_port_s = "";
		std::string seq_num_s = "";
		std::string ack_num_s = "";
		std::string offset_s = "";
		std::string flag_n_s = "";
		std::string window_s = "";
		std::string checksum_s = "";

		//toString for all numbers! The try catch is an artifact from before I changed this code, but whatever we'll keep it for now
		try{
			s_port_s = std::to_string(s_port);
			d_port_s = std::to_string(d_port);
			seq_num_s = std::to_string(seq_num);
			ack_num_s = std::to_string(ack_num);
			offset_s = std::to_string(offset);
			flag_n_s = std::to_string(flag);
			window_s = std::to_string(window);
			checksum_s = std::to_string(checksum);
		} catch (int e){
			cout << "bruh. error: " << e << endl;
		}

		//build string and return
		std::string s = "Sender Port: " + s_port_s + " \nDestination Port: " + d_port_s + " \nSequence Number: " + seq_num_s + " \nAcknowledgement Number: " + ack_num_s + " \nData Offset: " + offset_s;
		s = s + " \nPacket Flag: " + flag_n_s + "(" + getConstStr(flag) + ") " + " \nWindow Size: " + window_s + " \nChecksum Value: " + checksum_s + " \n";

		return s;
  }

};

//Populates head object from a JSON. Used by readHeader
tcp_header headData(tcp_header head, string json){

	//Stream the json
	std::stringstream ss;
	ss << json;

	//Load the string into a ptree
	pt::ptree reader;
	pt::read_json(ss, reader);

	//Set our data values for header object
	head.s_port = reader.get<uint16_t>("s_port");
	head.d_port = reader.get<uint16_t>("d_port");
	head.seq_num = reader.get<uint32_t>("seq_num");
	head.ack_num = reader.get<uint32_t>("ack_num");
	head.offset = reader.get<unsigned int>("offset");
	head.flag = reader.get<uint8_t>("flag");
	head.window = reader.get<uint16_t>("window");
	head.checksum = reader.get<uint16_t>("checksum");

	//return the head
	return (head);
}

//Create a tcp header from json. Returns a tcp_header object
tcp_header readHeader(string hdr){
  tcp_header head;

	head = headData(head, hdr);

  return head;
}


//TCP Packet Structure
struct tcp_packet {
	string header; //json of header
	string body;

	//Converts TCP Packet to a JSON containing string data and header JSON
	string toJson(){
		pt::ptree pkt;
		pkt.put("header", header);
		pkt.put("body", body);

		std::stringstream ss;
		pt::json_parser::write_json(ss, pkt);

		return ss.str(); //return JSON Packet
	}

	//toString for a packet. returns the header's toString as well as the string body
	string toString(){
		string s;
		s = "Header: \n\n" + readHeader(header).toString();
		s = s + " \nBody: " + body + "\n";
		return s;
	}
};


//Build a tcp_packet from json. Used by readPacket
tcp_packet packData(tcp_packet pack, string json){

	//Stream the json
	std::stringstream ss;
	ss << json;

	pt::ptree reader;
	pt::read_json(ss, reader);

	pack.header = reader.get<string>("header");
	pack.body = base64_decode(reader.get<string>("body"));

	return (pack);
}

//To read a full packet: header==> tcp_header p; string packet_json; p = readHeader(readPacket(packet_json).header);

//create packet from json. Returns a tcp_packet object to unpack
tcp_packet readPacket(string pkt){
	tcp_packet p;
	p = packData(p, pkt);

	return p;
}

//Unpacks a packet from json and populates data for the passed in head and body pointers
int unpack(string pck_json, tcp_header *head, string *body){
	try{
		tcp_packet pck = readPacket(pck_json);
		*head = readHeader(pck.header);
		*body = base64_decode(pck.body);
		return 0;
	} catch (int e) {
		cout << "Bruh Moment. Something broke: " << e << endl;
		return 1;
	}
}

typedef std::vector<tcp_packet> PacketStream;

//Validate integer from string
bool readIsInt(string input){
   for(int i = 0; i < input.length(); i++){
      if(isdigit(input[i]) == false)
         return false;
   }
   return true;
}

//Trim a string
string trim(string input){
	boost::trim_right(input);
	boost::trim_left(input);
	return input;
}


 //Generates a pseudo random number within the bounds passed in. Used to get the first sequence number for a transaction.
int genSeqNum(int lower, int upper){
	std::random_device rd; //device to generate randoms
	std::mt19937 gen(rd()); //seeding the generator. mt19937 is a pseudo random number generator
	std::uniform_int_distribution<> distr(lower, upper); //Defining the range
	return distr(gen); //Generate
}

//Gets a sequence number given the last number in sequence and the range to wrap on.
int getSeqNum(int last, int upper, int lower) {
	if(last == upper){
		return lower; //wrap
	} else{
		return last + 1; //normal case
	}

	//Shouldn't get here smh. this should crash if assigning to the uint.
	return -1;

}

//Gets a sequence number given the last number in sequence and the range to wrap on.
int lastSeqNum(int curr, int upper, int lower) {
	if(curr == lower){
		return upper; //wrap
	} else{
		return curr - 1; //normal case
	}

	//Shouldn't get here smh. this should crash if assigning to the uint.
	return -1;

}

//------------------begin server options---------------------//
struct srv_options {

	int proType;
	int packetSize;
	int timeout;
	int slidingWinSize;
	int seqLower;
	int seqUpper;
	int sitErrorInp;
	IntVec dropPacket;
	IntVec loseAck;

	//Converts server options to a JSON
	string toJson(){
		pt::ptree pkt;
		pkt.put("proType", proType);
		pkt.put("packetSize", packetSize);
		pkt.put("timeout", timeout);
		pkt.put("slidingWinSize", slidingWinSize);
		pkt.put("seqLower", seqLower);
		pkt.put("seqUpper", seqUpper);
		pkt.put("sitErrorInp", sitErrorInp);

		pt::ptree dropPacketArr;
		int j = 0;

		for(int i : dropPacket){
			pt::ptree element;
			element.put(std::to_string(j), i);
			dropPacketArr.push_back(pt::ptree::value_type("", element));
			j++;
		}

		pkt.put_child("dropPacket", dropPacketArr);


		pt::ptree loseAckArr;
		j = 0;

		for(int i : loseAck){
			pt::ptree element2;
			element2.put(std::to_string(j), i);
			dropPacketArr.push_back(pt::ptree::value_type("", element2));
			j++;
		}

		pkt.put_child("loseAck", loseAckArr);

		std::stringstream ss;
		pt::json_parser::write_json(ss, pkt);

		return ss.str(); //return JSON Options
	}

	string toString(){
		string c = "Server Options: \n\nProtocol Type: " + std::to_string(proType) + " \nPacket Size: " + std::to_string(packetSize) + " \nTimeout Interval: " + std::to_string(timeout) + " \nSliding Window Size: " + std::to_string(slidingWinSize);
		c = c + " \nSequence Range: " + std::to_string(seqLower) + " to " + std::to_string(seqUpper) + " inclusive." + " \nType of Situational Error: " + std::to_string(sitErrorInp) + "\n\n";


		if(sitErrorInp == 3) {
			c = c + "Packet numbers to be lost: ";
			for(int n : dropPacket){
				c = c + "" + std::to_string(n) + " ";
			}

			c = c + "\n";

			c = c + "Ack numbers to be lost: ";
			for(int n : loseAck) {
				c = c + "" + std::to_string(n) + " ";
			}

			c = c + "\n";
		}


		return c;
	}

}; //end struct

//Build server options from json. Used by readSrvOp
srv_options srvOpData(srv_options srv, std::string json){

	//Stream the json
	std::stringstream ss;
	ss << json;

	pt::ptree reader;
	pt::read_json(ss, reader);

	srv.proType = reader.get<int>("proType");
	srv.packetSize = reader.get<int>("packetSize");
	srv.timeout = reader.get<int>("timeout");
	srv.slidingWinSize = reader.get<int>("slidingWinSize");
	srv.seqLower = reader.get<int>("seqLower");
	srv.seqUpper = reader.get<int>("seqUpper");
	srv.sitErrorInp = reader.get<int>("sitErrorInp");

	IntVec dropP;

	pt::ptree &array = reader.get_child("dropPacket");
	int j = 0;
	for(pt::ptree::iterator element = array.begin(); element != array.end(); element++){
		dropP.push_back(element->second.get<int>(std::to_string(j)));
		j++;
	}

	srv.dropPacket = dropP;


	IntVec loseA;

	pt::ptree &array2 = reader.get_child("loseAck");
	j = 0;
	for(pt::ptree::iterator element = array2.begin(); element != array2.end(); element++){
		loseA.push_back(element->second.get<int>(std::to_string(j)));
		j++;
	}

	srv.loseAck = loseA;


	return (srv);
}

//create srv options struct from json
srv_options readSrvOp(string srv) {
	srv_options s;
	s = srvOpData(s, srv);

	return s;
}

//------------------end server options---------------------//

//Write to a file. Will change this to take a vector of strings
int writeFile(PacketStream *packets, string fPath){
	try {
		cout << "Writing file..." << endl;
		ofstream outfile;
		outfile.open(fPath, ios::out | ios::binary);


		std::vector<char> file;
		std::string t_bod = "";

		for(tcp_packet p : *packets){
			t_bod = p.body;
			t_bod = base64_decode(t_bod);
			for(char c : t_bod){
				file.push_back(c);
			}
		}


		outfile.write (&file[0], file.size());

		outfile.close();

		cout << "Successfully saved the file to: " << fPath << endl;
		return 0;
	} catch(int e){
		cout << "Uh oh! Error code:" << e << endl;
		return e;
	}
}


  // String a will be the current sum of the bits
	// String b is what will be added to the sum
	std::string sumNum(std::string a, std::string b, int *numCarries) {
		std::string sum = ""; // This will be the sum result
		int carry = 0;
		int size = 16;

		for (int i = size-1; i >= 0; i--) {
			int bitA = a.at(i) - '0';
			int bitB = b.at(i) - '0';
			int bit = (bitA ^ bitB ^ carry) + '0';

			sum = (char)bit + sum;
			carry = (bitA & bitB) | (bitB & carry) | (bitA & carry);
		}


		// This wraps the bits that have overflowed (if there are multiple 
		//carries, all happen at once)
		if (carry) { // 1001 + 1000 = 10001 -> '1' + '0001'
			numCarries += 1;
		}

		return sum;
	}

	// Take the one's compliment of the given string of bits (1001 -> 0110)
	std::string onesCompliment(std::string str) {
		std::string complimentStr = "";

		for (int i = 0; i < str.length(); i++) {
			if (str[i] == '0') {
				complimentStr += "1";
			} else if (str[i] == '1') {
				complimentStr += "0";
			}
		}

		return complimentStr;
	}
	
	bool validateChecksum(tcp_header curr, int packetSize){
			// Validate checksum
			std::string sumNumTotal = "";
			std::bitset<16> emptyBits; // all 16 bits initialized to 0 for initial sum

			std::bitset<16> currS_Port(curr.s_port);
			std::bitset<16> currD_Port(curr.d_port);
			std::bitset<16> currSeq_Num(curr.seq_num);
			std::bitset<16> currOffset(curr.offset);
			std::bitset<16> currFlag(curr.flag);
			std::bitset<16> currWindow(curr.window);
			std::bitset<16> currChecksum(curr.checksum);
			std::bitset<16> currSize(packetSize);


			int numCarries = 0; // How many times the sum will wrap/carry over

			// Takes the sum of the packet headers 
			sumNumTotal = sumNum(emptyBits.to_string(), currS_Port.to_string(), &numCarries);
			sumNumTotal = sumNum(sumNumTotal, currD_Port.to_string(), &numCarries);
			sumNumTotal = sumNum(sumNumTotal, currSeq_Num.to_string(), &numCarries);
			sumNumTotal = sumNum(sumNumTotal, currOffset.to_string(), &numCarries);
			sumNumTotal = sumNum(sumNumTotal, currFlag.to_string(), &numCarries);
			sumNumTotal = sumNum(sumNumTotal, currWindow.to_string(), &numCarries);
			sumNumTotal = sumNum(sumNumTotal, currSize.to_string(), &numCarries);

			std::bitset<16> carriesBin(numCarries);

			sumNumTotal = sumNum(sumNumTotal, carriesBin.to_string(), &numCarries);
			
			
			numCarries = 0; 

			// Find the checksum value (the one's compliment of the sumNumTotal)
			std::string complimentChecksum = onesCompliment(sumNumTotal);
			
			/*
			// Sum the sumNumTotal and complimentChecksum value
			std::string checksum = sumNum(sumNumTotal, complimentChecksum, &numCarries);

			// Take the one's compliment of the checksum value (should be 0 if no errors, anything else is an error)
			std::string checksumValue = onesCompliment(checksum);
			std::bitset<16> checksumBits(checksumValue);
			
			
			//cout << "" << << endl;
			cout << "Checksum Value  = " << complimentChecksum << endl;
			cout << "summed = " << checksum << endl; //should = 1111111111111111
			cout << "after onesCompliment " <<  checksumValue << endl; //should = 0000000000000000
			
			// If there was an error in the transmission
			if (checksumBits.to_ulong() != 0) {
				return false;
			} else { 
				return true;
			}
			*/
			
			
			//cout << "Checksum Value  = " << complimentChecksum << endl;
			//cout << "original value = " << currChecksum << endl;
			
			return complimentChecksum == currChecksum.to_string();
			
	}




//----------------------------------------------Begin Client------------------------------------------------------//
srv_options srvOp;

class client
{
public:
  client(boost::asio::io_context& io_context)
    : stopped_(false),
      socket_(io_context),
      deadline_(io_context),
      heartbeat_timer_(io_context)
  {
  }

  // Called by the user of the client class to initiate the connection process.
  // The endpoints will have been obtained using a tcp::resolver.
  void start(tcp::resolver::results_type endpoints)
  {
    // Start the connect actor.
    endpoints_ = endpoints;
    start_connect(endpoints_.begin());

    // Start the deadline actor. You will note that we're not setting any
    // particular deadline here. Instead, the connect and input actors will
    // update the deadline prior to each asynchronous operation.
    deadline_.async_wait(boost::bind(&client::check_deadline, this));
  }

  // This function terminates all the actors to shut down the connection. It
  // may be called by the user of the client class, or by the class itself in
  // response to graceful termination or an unrecoverable error.
  void stop()
  {
    stopped_ = true;
    bs::error_code ignored_ec;
    socket_.close(ignored_ec);
    deadline_.cancel();
    heartbeat_timer_.cancel();
  }

private:
  void start_connect(tcp::resolver::results_type::iterator endpoint_iter)
  {
    if (endpoint_iter != endpoints_.end())
    {
      std::cout << "Trying " << endpoint_iter->endpoint() << "...\n";

      // Set a deadline for the connect operation.
      deadline_.expires_after(boost::asio::chrono::seconds(60));

      // Start the asynchronous connect operation.
      socket_.async_connect(endpoint_iter->endpoint(),
          boost::bind(&client::handle_connect, this,
            boost::placeholders::_1, endpoint_iter));
    }
    else
    {
      // There are no more endpoints to try. Shut down the client.
      stop();
    }
  }

  void handle_connect(const bs::error_code& ec,
      tcp::resolver::results_type::iterator endpoint_iter)
  {
    if (stopped_)
      return;

    // The async_connect() function automatically opens the socket at the start
    // of the asynchronous operation. If the socket is closed at this time then
    // the timeout handler must have run first.
    if (!socket_.is_open())
    {
      std::cout << "Connect timed out\n";

      // Try the next available endpoint.
      start_connect(++endpoint_iter);
    }

    // Check if the connect operation failed before the deadline expired.
    else if (ec)
    {
      std::cout << "Connect error: " << ec.message() << "\n";

      // We need to close the socket used in the previous connection attempt
      // before starting a new one.
      socket_.close();

      // Try the next available endpoint.
      start_connect(++endpoint_iter);
    }

    // Otherwise we have successfully established a connection.
    else
    {
      std::cout << "Connected to " << endpoint_iter->endpoint() << "\n";


	cout << "Getting Server Configuration.." << endl;


	//Load configuration
	string configJson = read_();
	send_("ACK");

	srvOp = readSrvOp(configJson);
	cout << endl << "Configuration Loaded! Current Config: " << endl << endl << srvOp.toString() << endl;
	start_read();
	
	sendAck = false; //we don't want to send acks just yet. We will when we have to
	
	winSize = srvOp.slidingWinSize;
	win_start = 0;
	win_end = winSize - 1;
	curr_frame = 0;

	currAck = 1;
	protocol = srvOp.proType;
	
	seq_curr = 0;
	seq_last = 0;
	packInd = 0;
	
	seqLow = srvOp.seqLower;
	seqHi = srvOp.seqUpper;

	cout << "Reading... " << endl;
	  //read_ returns the json of a packet
	//PacketStream packs = fileread_();

	//cout << "Read complete." << endl;

    }
  }

    //Read a string from a socket until it hits our delim. This is synchronous/blocking and is used to load the server configuration.
string read_() {
       boost::asio::streambuf buf;
	   boost::system::error_code error;
	   string data;

       boost::asio::read_until( socket_, buf, delim, error);
	if( error && error != boost::asio::error::eof ) {
		cout << "receive failed: " << error.message() << endl;
	} else {
       data = boost::asio::buffer_cast<const char*>(buf.data());

	   data.pop_back();
	   //data.pop_back();
		//cout << "data before decode: " << data << endl;
	   //decode data
	   try{
		data = base64_decode(data);
	   } catch (std::exception e) {
		   //cout << "dang " << endl;
		   return data;
	   }

	   //cout << "Read: " << data << endl;
	}


	return data;
}

//starts/renews a timer on the read handler
  void start_read()
  {
	 //cout << "starting read... " << endl;
	//set timeout
		deadline_.expires_after(boost::asio::chrono::seconds(srvOp.timeout)); //set deadline
		//cout << "Deadline of " << srvOp.timeout << "seconds " << endl;

	//call async read until, giving it our read handler
    boost::asio::async_read_until(socket_,
        boost::asio::dynamic_buffer(input_buffer_), delim,
        boost::bind(&client::handle_read, this,
          boost::placeholders::_1, boost::placeholders::_2));
  }



/* For reference, shit we initialized in this class
  
	//init window 
	int winSize = srvOp.slidingWinSize;
	int win_start = 0;
	int win_end = winSize - 1;
	int curr_frame = 0;

	int currAck = 1;

*/


	//Read handler. We are only receiving packets, so this will parse them and do what we need to do.
  void handle_read(const bs::error_code& ec, std::size_t n)
  {
	  tcp_packet curr_pack;
	  tcp_header curr_head;

		if(timed == true){
			cout << "We timed out! handle it here..." << endl;

			std::string line(input_buffer_.substr(0, n - 1));
			input_buffer_.erase(0, n);
			line = "";
			input_buffer_ = "";
			//cout << "cleared input buffer: " << input_buffer_ << endl;


			//HANDLE CLI SIDE TIMEOUT

			timed = false;
			start_read();
			return;
		} else {
	  //cout << "Got somehting" << endl;
	  string ackit = getConstStr(ACK);
	  string finish = getConstStr(FIN);
    if (stopped_)
      return;

    if (!ec) {
	//load the data
      // Extract the newline-delimited message from the buffer.
      std::string line(input_buffer_.substr(0, n - 1));
      input_buffer_.erase(0, n);
	  
	  //cout << "line before: " << line << endl;
	
	try {
		line = base64_decode(line);
	} catch (std::exception e) {
		cout << "baddy" << endl;
		start_read();
		return;
	}
	cout << "line decoded: " << line << endl;


	  //line is now the string we were sent!
	  if(line == "EXIT"){
			cout << "getting outta here" << endl;
		socket_.close();
		stop();
	  } else if(line == finish){ //we should finish up
		send_(ackit); //send an ack since this part kills the program
		cout << endl << "Finished up. On to the next thing." << endl;
		writeFile(&packets, "client_out");
		
		//OUTPUT
		printOutput();
		
		//close client
		socket_.close();
		stop();

	   } else { //Default case. This is a packet so read it into packets
			bool isvalid;
			bool resended = false;
			curr_pack = readPacket(line);
			curr_head = readHeader(curr_pack.header);
			//DO STUFF
			
			//validate checksum
			isvalid = validateChecksum(readHeader(curr_pack.header), srvOp.packetSize);
			
			if(isvalid){
				//cout << "valid " << endl;
			} else {
				cout << "not valid checksum! " << endl;
			}
			
			
			//check seq nums
			seq_curr = (uint32_t)curr_head.seq_num;
			
			//cout << "current seq num: " << seq_curr << endl;

			if(seq_last == 0){
				seq_last = lastSeqNum(seq_curr, seqHi, seqLow);
			} else {
				int expectedlast = lastSeqNum(seq_curr, seqHi, seqLow);
				//cout << "Last: " << seq_last << " and expected: " << expectedlast << endl;
				if(seq_last != expectedlast){ //we missed something!!
					std::string aya = "";
					std::string linea = "";
					size_t len;
					string readit;
					
					switch(protocol){
						case 1: //GBN
							//resend
							send_("RESEND"); //tell server to resend.
							resended = true;
							//pop back entire frame
							cout << "here" << endl;
							
							for(int f = curr_frame; f > win_start; f--){
								packets.pop_back();
								cout << "f = " << f << endl;
							}
							
							//reinit the window and frame
							curr_frame = win_start;
							
							cout << "shift frame back to " << curr_frame << endl;
													
							while(aya != "HOLUP"){
								cout << "waiting....." << endl;
								aya = read_();
								send_("GIVEMEHOLUP");
								//clear input
							}
							
							cout << "got here!" << endl;
							send_("GO");
							
							sendAck = false;
							start_read();
							break;
						case 2: //SR
							break;
					}
				}//end missed
			}
			
			//cout << "window end: " << win_end << endl;
			//frame shift 	do we need an ack on this one?
			if(curr_frame == win_end){ //current frame is the final in the window
				sendAck = true; //we need this, commented out for the time being for testing
				
				//shift to next state
				cout << "shifting beginning of window to " << (win_start + winSize) << endl; 
				win_start += winSize; //move to next frame outside of the window
				win_end += winSize;
				curr_frame = win_start;
				
			} else {
				cout << "shift from " << curr_frame << " to " << (curr_frame + 1) << endl;
				curr_frame++; //move right
			}
			
			
			//Handle stuff
			
			
			
		//Send ack 
		  if (!line.empty() && sendAck == true) { //we should send an ack as main program! so let's iterate the ack num too after we do so
			//std::cout << "Received: " << line << "\n";
				send_(ackit);
				currAck++;
				sendAck = false;
		  }	
			seq_last = seq_curr; //set last
			//cout << "last after: " << seq_last;
			packets.push_back(curr_pack);
			//packets[packInd] = curr_pack;
			packInd++;
			//send_(ackit);
			start_read();
	   }
	

    } else {
	if( ec != boost::asio::error::eof)
      std::cout << "Error on receive: " << ec.message() << "\n";

      stop();
    }
	}
  } //end handle read

    void send_(string message_) {
	  message_ = base64_encode(message_);
	  message_ += delim;

	  //cout << "Sending " << message_ << endl;

        socket_.async_send(boost::asio::buffer(message_),
            boost::bind(&client::handle_write, this,
            boost::asio::placeholders::error));
    /*boost::asio::async_write(socket_, boost::asio::buffer(message_, message_.size()),
        boost::bind(&client::handle_write, this, boost::placeholders::_1));*/
  }

  void start_write()
  {
    if (stopped_)
      return;

    /*// Start an asynchronous operation to send a heartbeat message.
    boost::asio::async_write(socket_, boost::asio::buffer(std::to_string(delim), 1),
        boost::bind(&client::handle_write, this, boost::placeholders::_1));*/
  }

  void handle_write(const bs::error_code& ec)
  {
    if (stopped_)
      return;

    if (!ec)
    {
      // Wait 10 seconds before sending the next heartbeat.
      //heartbeat_timer_.expires_after(boost::asio::chrono::seconds(10));
      //heartbeat_timer_.async_wait(boost::bind(&client::start_write, this));
    }
    else
    {
      std::cout << "Error on write: " << ec.message() << "\n";

      stop();
    }
  }

  void check_deadline()
  {
    if (stopped_)
      return;

    // Check whether the deadline has passed. We compare the deadline against
    // the current time since a new asynchronous operation may have moved the
    // deadline before this actor had a chance to run.
    if (deadline_.expiry() <= steady_timer::clock_type::now())
    {
		cout << "Timed out" << endl;

      // The deadline has passed. The socket is closed so that any outstanding
      // asynchronous operations are cancelled.
			socket_.cancel();
		send_("TIMEOUT");
		timed = true;
		//socket_.close();
		//stop();
		//return;

      // There is no longer an active deadline. The expiry is set to the
      // maximum time point so that the actor takes no action until a new
      // deadline is set.
		deadline_.expires_at(steady_timer::time_point::max());
    }

    // Put the actor back to sleep.
    deadline_.async_wait(boost::bind(&client::check_deadline, this));
  }

  void printOutput() {
		cout << "Last packet seq# received: " << lastPcktSeqNum << endl;
		cout << "Number of original packets received: " << originialPackets << endl;
		cout << "Number of retransmitted packets received: " << retransmittedPackets << endl;
	}

private:
  bool stopped_;
	bool timed;
	bool sendAck;
  tcp::resolver::results_type endpoints_;
  tcp::socket socket_;
  std::string data;
  std::string input_buffer_;
  steady_timer deadline_;
  steady_timer heartbeat_timer_;
  PacketStream packets;
  
  int seqLow;
  int seqHi;
  
  int seq_last;
  int seq_curr;
  
	//init window 
	int winSize;
	int win_start;
	int win_end;
	int curr_frame;
	
	int packInd;

	int currAck;

	// Declare the output variables
	uint32_t lastPcktSeqNum;
	int originialPackets;
	int retransmittedPackets;
	
	int protocol;
//protocol numbers:
// 1 -> GBN
// 2 -> SR
	

};



int main(int argc, char* argv[])
{
  try
  {
    boost::asio::io_context io_context;
    tcp::resolver r(io_context);
    client c(io_context);

    c.start(r.resolve(HOST, std::to_string(PORT)));

    io_context.run();
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
