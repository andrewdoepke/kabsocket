#include <iostream>
#include <ctime>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <random>
#include "base64.h"
#include <boost/date_time/posix_time/posix_time.hpp>

using namespace boost::asio;
namespace bs = boost::system;
using ip::tcp;
using std::string;
using std::cout;
using std::cin;
using std::endl;
using std::ios;
using std::ofstream;

const uint16_t PORT = 1234; //Simulated port for this program

namespace pt = boost::property_tree;

typedef std::vector<int> IntVec;
typedef std::vector<string> StrVec;

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

	const int header_size = 17; //17 byte headers

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
	pack.body = reader.get<string>("body");

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
		*body = pck.body;
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
	} else if(last >= lower && last < upper){
		return last + 1; //normal case
	}

	//Shouldn't get here smh. this should crash if assigning to the uint.
	return -1;

}

//Struct to hold server options that are prompted for in user input
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

		//Push back every dropped packet and acks lost. These will go to arrays which will be translated back when reading.
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

	pt::ptree &array2 = reader.get_child("dropPacket");
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


//User input function! Takes a srv_options struct as a parameter, prompts for and loads values, and returns the new struct
srv_options userInput(srv_options server_options){
	int proType = 0;
	int packetSize = 0;
	int timeout = -1; //set to -1 to allow another option on 0
	int slidingWinSize = 0;
	int seqLower = -1;
	int seqUpper = -1;

	int sitErrorInp = 0; //menu input for this. Also is the type of errors to be generated.
	char sitErrors; //Y or N character for menu.

	std::vector<int> dropPacket;
	std::vector<int> loseAck;

	string inp = "";
	int tempVal = 0;


//------------------user input------------------//

//GBN or SR?
	while(proType < 1 || proType > 2){
		cout << "Would you like to use GBN or SR protocol? 1 for GBN, 2 for SR." << endl;
		cin >> inp;
		if(readIsInt(inp)){//parse the int
			proType = stoi(inp);
		}
		if(proType < 1 || !readIsInt(inp) || proType > 2){
			cout << "Error! Invalid input. Please try again or CTR-C to quit." << endl;
		}
	}


//Packet size
	while(packetSize < 1){
		cout << "Please enter packet size (in bytes): " << endl;
		cin >> inp;
		if(readIsInt(inp)){//parse the int
			packetSize = stoi(inp);
		}
		if(packetSize < 1 || !readIsInt(inp)){
			cout << "Error! Invalid input. Please try again or CTR-C to quit." << endl;
		}
	}

	if(packetSize <= header_size){
		cout << "Minimum packet size is 20 bytes (header is 17). Defaulting to 20 bytes." << endl;
		packetSize = 18;
	}

//Timeout interval (user-specified or ping-calculated)
	while(timeout < 0){
		cout << "Please enter the timeout interval you'd like, as a positive integer. Enter 0 for ping-based timeout." << endl;
		cin >> inp;
		if(readIsInt(inp)){//parse the int
			timeout = stoi(inp);
		}

		if(timeout == 0){
			cout << "Using a ping-based timeout interval" << endl; //calculate this per connection, so we can do this when we open each connection
			//continue loop
			//BIG TODO: Make sure we check for this later instead of having a 0 timeout
		} else if(timeout < 0 || !readIsInt(inp)){ //otherwise bad input so loop
			cout << "Error! Invalid input. Please try again or CTR-C to quit." << endl;
		}
	}

//Sliding window size
	while(slidingWinSize < 1){
		cout << "Please enter sliding window size (in bytes): " << endl;
		cin >> inp;
		if(readIsInt(inp)){//parse the int
			slidingWinSize = stoi(inp);
		}
		if(slidingWinSize < 1 || !readIsInt(inp)){
			cout << "Error! Invalid input. Please try again or CTR-C to quit." << endl;
		}
	}

//Range of sequence numbers
	while(seqLower < 0){
		cout << "Please enter the lower bound of the sequence number range: " << endl;
		cin >> inp;
		if(readIsInt(inp)){//parse the int
			seqLower = stoi(inp);
		}
		if(seqLower < 0 || !readIsInt(inp)){
			cout << "Error! Invalid input. Please try again or CTR-C to quit." << endl;
			seqLower = -1;
		}
	}

	while(seqUpper < 0){
		cout << "Please enter the upper bound of sequence number range: " << endl;
		cin >> inp;
		if(readIsInt(inp)){//parse the int
			seqUpper = stoi(inp);
		}
		if(seqUpper < 0 || !readIsInt(inp) || seqUpper < seqLower){
			cout << "Error! Invalid input. Please try again or CTR-C to quit." << endl;

			if(seqUpper < seqLower){
				cout << "\nPlease enter something greater or equal to " << seqLower << endl << endl;
			}
			seqUpper = -1;
		}
	}


//Situational errors (none, randomly generated, or user-specified, i.e., drop packets 2, 4, 5, lose acks 11, etc.)


	while(sitErrorInp < 1){
		cout << "What kind of situational errors would you like to simulate? \n 1 for none \n 2 for randomly generated errors \n 3 to specify errors manually" << endl;
		cin >> inp;
		if(readIsInt(inp)){//parse the int
			sitErrorInp = stoi(inp);
		}
		if(sitErrorInp < 1 || !readIsInt(inp)){
			cout << "Error! Invalid input. Please try again or CTR-C to quit." << endl;
		}
	}

	//Handle Values

	if(sitErrorInp == 2){ //Randomly Generated Errors
		cout << "Randomly Generating Errors" << endl;
		//TODO: randomly generate errors




	} else if(sitErrorInp == 3){ //User Specified Errors

		//prompt for packet loss and Infinitely prompt for drop packets until done

		cout << "Would you like to drop any packets? (Y/N): " << endl;
		cin >> inp;
		if(inp.length() != 1 || toupper(inp[0]) != 'Y'){
			sitErrors = 'N';
		} else {
			sitErrors = toupper(inp[0]);
		}

		while (sitErrors == 'Y') {
			while(tempVal < 1){
				cout << "Please enter a packet number that you would like to drop: " << endl;
				cin >> inp;
				if(readIsInt(inp)){//parse the int
					tempVal = stoi(inp);
				}

				if(tempVal < 1 || !readIsInt(inp)){
					cout << "Error! Invalid input. Please try again or CTR-C to quit." << endl;
				} else {
					dropPacket.push_back(tempVal);
				}

			}

			tempVal = 0; //re-init to keep looping.

			cout << "Would you like to drop any other packets? (Y/N): " << endl;
			cin >> inp;

			if(inp.length() != 1 || toupper(inp[0]) != 'Y'){
				sitErrors = 'N';
			} else {
				sitErrors = toupper(inp[0]);
			}


		} //end while 1

		std::sort(dropPacket.begin(), dropPacket.end()); //sort so packets are in order

		//Infinitely prompt to lose acks until done
		cout << "Would you like to lose any acks? (Y/N): " << endl;
		cin >> inp;
		if(inp.length() != 1 || toupper(inp[0]) != 'Y'){
			sitErrors = 'N';
		} else {
			sitErrors = toupper(inp[0]);
		}

		while (sitErrors == 'Y') {
			while(tempVal < 1){
				cout << "Please enter an ack number that you would like to lose:" << endl;
				cin >> inp;
				if(readIsInt(inp)){//parse the int
					tempVal = stoi(inp);
				}

				if(tempVal < 1 || !readIsInt(inp)){
					cout << "Error! Invalid input. Please try again or CTR-C to quit." << endl;
				} else { //good so use this value
					loseAck.push_back(tempVal);
				}
			}

			tempVal = 0; //re-init to keep looping.

			cout << "Would you like to drop any other acks? (Y/N): ";
			cin >> inp;
			if(inp.length() != 1 || toupper(inp[0]) != 'Y'){
				sitErrors = 'N';
			} else {
				sitErrors = toupper(inp[0]);
			}


		}//end while 2

		std::sort(loseAck.begin(), loseAck.end()); //Sort so they are in order

	} else {
		cout << "No errors simulated." << endl;
		//Do nothing :)
	}

	//at this point all values are loaded, and we can test in runtime for packets/acks sent to determine whether or not a packet/ack should be an error, if it gets to that packet/ack
	//if(pCnt = loseAck[ackCnt]) {... ackCnt++;}
	//if(pCnt = dropPacket[dropCnt]) {... dropCnt++;}


	//Set Struct data with all of this!

	server_options.proType = proType;
	server_options.packetSize = packetSize;
	server_options.timeout = timeout;
	server_options.slidingWinSize = slidingWinSize;
	server_options.seqLower = seqLower;
	server_options.seqUpper = seqUpper;
	server_options.sitErrorInp = sitErrorInp;
	server_options.dropPacket = dropPacket;
	server_options.loseAck = loseAck;

	return server_options;
} //end user input

//Found this code to load binary from a file
std::vector<char> readBinaryFile(string fPath) {
  std::ifstream in(fPath, std::ios::binary);
  in.seekg(0, std::ios::end);
  int iSize = in.tellg();
  in.seekg(0, std::ios::beg);

  std::vector<char> pBuff(iSize);
  if ( iSize > 0 )
      in.read(&pBuff[0], iSize);
  in.close();

	return pBuff;
}


//Changed to use vector of strings containing each packet body
void stageFile(int packetSize, std::vector<char> *buff, StrVec *out) {
	int i;
	string tempbod = "";
	int bC = 0;
	int buffSize = buff->size();
	int progCount = buffSize;
	int bodySize = packetSize - header_size;

	   while(bC < buffSize){ //split packets
			 tempbod="";

			 if(bC + bodySize <= buffSize){
				tempbod = string(buff->begin() + bC, buff->begin() + bC + bodySize);
				bC += bodySize;
				progCount -= bodySize;
			 } else {
				tempbod = string(buff->begin() + bC, buff->end());
				bC = buffSize;
				progCount = 0;
			 }
			 cout << "Bytes left to go: " << progCount << endl;

			 out->push_back(tempbod);
	   }//end while
}

//Batch convert a PacketStream(Vector<tcp_packet>) to a StrVec(Vector<string>) containing json for each packet
//we might need this
StrVec convert_PacketStream_StrVec(PacketStream packets){
	StrVec out;

	for (tcp_packet p : packets){
		out.push_back(p.toJson());
	}

	return out;
}


tcp_header initHeader(srv_options *srvOp){
	tcp_header base_header;
	base_header.seq_num = genSeqNum(srvOp->seqLower, srvOp->seqUpper); //generate first seq number
	base_header.ack_num = 1; //initialize ack num
	base_header.offset = 0; //initialize offset
	base_header.flag = IGN; //default flag to ignore
	base_header.window = srvOp->slidingWinSize; //initialize window size. Probably will have to changed
	base_header.checksum = 0; //initialize checksum

	return base_header;

}

void advanceHeader(tcp_header *last, srv_options *srvOp, uint8_t flag){
	if(last->ack_num > 1){
		last->seq_num = getSeqNum(last->seq_num, srvOp->seqLower, srvOp->seqUpper);
		last->ack_num++;
		last->flag = flag;
		//add more
	}
}


//----------------------------------------------Begin Server------------------------------------------------------//

srv_options srvOp;
StrVec bodies;

  void loadFile(srv_options *options,string filePath){
	  	std::vector<char> buff = readBinaryFile(filePath);
		stageFile(options->packetSize, &buff, &bodies); //load bodies from the file
  }

using boost::asio::ip::tcp;

std::string make_daytime_string()
{
  using namespace std; // For time_t, time and ctime;
  time_t now = time(0);
  return ctime(&now);
}

class tcp_connection
  : public boost::enable_shared_from_this<tcp_connection>
{
public:
  typedef boost::shared_ptr<tcp_connection> pointer;

  static pointer create(boost::asio::io_context& io_context)
  {
    return pointer(new tcp_connection(io_context));
  }

  tcp::socket& socket()
  {
    return socket_;
  }

  void start() {

	  cout << "Connected!" << endl;

	//start_read();

	cout << "Sending Server Config" << endl;
	send_(srvOp.toJson()); //sends the json of the srvConf
	cout << "Sent!" << endl;

	string val = "";

	while(val != "ACK"){
		val = read_();
		if(val=="TIMEOUT"){
			cout << "Timed out! sad." << endl;
			//handle timeout
		}
	}

	//cout << "Back in parent: " << val << endl;

	cout << "Sending the file..." << endl;
	std::vector<char> data;
	filesend_(&srvOp);
  }

  

  //Read a string from a socket until it hits our delim
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

	   //decode data
	   data = base64_decode(data);

	   //cout << "Read: " << data << endl;
	}
	return data;
}


  void send_(string message_) {


	  message_ = base64_encode(message_);
	  message_ += delim;

	  //cout << "Sending " << message_ << endl;
	  cout << "Sent a packet" << endl;

	  boost::asio::async_write(socket_, boost::asio::buffer(message_),
        boost::bind(&tcp_connection::handle_write, shared_from_this(),
          boost::asio::placeholders::error,
          boost::asio::placeholders::bytes_transferred));
  }

  //Send the file
    void filesend_(srv_options *options) {
		string ackit = getConstStr(ACK);
		string finish = getConstStr(FIN);

		string validate = "";
		string exit = "EXIT";

		//Write all packets individually
		string tempPack;
		tcp_header curr_head = initHeader(options);
		tcp_packet curr_packet;

		cout << "Ready to send" << endl;


		for(string b : bodies){
			advanceHeader(&curr_head, options, IGN); //advance the header. This raises the ack number by 1 and iterates the sequence number

			curr_packet.body = base64_encode(b); //encode the body..
			curr_packet.header = curr_head.toJson(); //Set the current packet header

			tempPack = curr_packet.toJson();

			validate = "";

			send_(tempPack);

			cout << "waiting for ack..." << endl;


			while(validate != ackit){ //wait for an ack
				validate = read_();
				if(validate=="TIMEOUT"){
					cout << "Timed out! sad." << endl;

					//HANDLE SERVER SIDE TIMEOUT
					send_(exit); //This will kill the client with exit command
					return;
					//handle timeout
				}
			}
		}

	   cout << "Sent! Telling client to exit." << endl;
	   send_(finish);

	   	validate = "";
		cout << "waiting for ACK to end..." << endl;
		while(validate != ackit){
			validate = read_();
			if(validate=="TIMEOUT"){
				cout << "Timed out! sad." << endl;
				boost::asio::write( socket_, boost::asio::buffer("\0") );
				send_(exit);
				return;
				//handle timeout
			}
		}

	  cout << "Finished processing this one." << endl << endl;
  }

private:
  tcp_connection(boost::asio::io_context& io_context)
    : socket_(io_context)
  {
  }

  void handle_write(const boost::system::error_code& error,
      size_t written/*bytes_transferred*/) {
	//cout << "Wrote " << written << endl;


  }



std::string input_buffer_;
string thisguy;
  tcp::socket socket_;
  std::string message_;

};

class tcp_server
{
public:
  tcp_server(boost::asio::io_context& io_context)
    : io_context_(io_context),
      acceptor_(io_context, tcp::endpoint(tcp::v4(), PORT))
  {
    start_accept();
  }

private:
  void start_accept() {

	cout << "Waiting for a connection..." << endl;

    tcp_connection::pointer new_connection =
      tcp_connection::create(io_context_);

    acceptor_.async_accept(new_connection->socket(),
        boost::bind(&tcp_server::handle_accept, this, new_connection,
          boost::asio::placeholders::error));
  }

  void handle_accept(tcp_connection::pointer new_connection,
      const boost::system::error_code& error)
  {
    if (!error)
    {
      new_connection->start();
    }

    start_accept();
  }

  boost::asio::io_context& io_context_;
  tcp::acceptor acceptor_;
};

int main() {
	//Take user input
	srvOp = userInput(srvOp);
	cout << "Loading File..." << endl;
	string filePath = "100mB";
	loadFile(&srvOp, filePath);
  try {
    boost::asio::io_context io_context;
    tcp_server server(io_context);
    io_context.run();
  }
  catch (std::exception& e)
  {
    std::cerr << e.what() << std::endl;
  }

  return 0;
}
