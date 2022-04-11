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

using namespace boost::asio;
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
	} else if(last >= lower && last < upper){
		return last + 1; //normal case
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

//------------------end server options---------------------//

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

std::string binToString(std::vector<char> pBuff){
	std::string out(pBuff.begin(), pBuff.end());
	return out;
}

//Encode a file in base 64
string b64EncodeFile(string fPath){
	string encoded = "";

	std::vector<char> raw = readBinaryFile(fPath);

	string mid = binToString(raw);
	cout << "Mid: " << mid << endl;

	encoded = base64_encode(mid);
	cout << "Encoded: " << endl << encoded << endl;

	return encoded;
}

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


string read_(tcp::socket & socket, boost::asio::streambuf & receive_buffer) {

		std::string fin = "";
		boost::system::error_code error;

		std::string data;

		boost:asio:read_until(socket, receive_buffer, delim, error);

		if( error && error != boost::asio::error::eof ) {
			cout << "receive failed: " << error.message() << endl;
		}
		else {
			std::istream is(&receive_buffer);

			while(std::getline(is, data)) {
				fin = fin + data;
			}

		}

	//fin.pop_back(); //remove eto
	fin.pop_back(); //remove etx

	fin = base64_decode(fin);

    return fin; //return final data
}

void send_(tcp::socket & socket, const string& message) {
	const string msg = base64_encode(message) + delim;
	boost::system::error_code error;
    boost::asio::write( socket, boost::asio::buffer(msg), error );

    if( !error ) {
       //cout << "Sent: " << message << endl;
	   cout << "Sent." << endl;
    }
    else {
       cout << "send failed: " << error.message() << endl;
    }

       //const string msg = message + "|";
       //boost::asio::write( socket, boost::asio::buffer(message) );
}

std::string make_string(boost::asio::streambuf& streambuf) {
  return {boost::asio::buffers_begin(streambuf.data()),
          boost::asio::buffers_end(streambuf.data())};
}


PacketStream fileread_(tcp::socket & socket, boost::asio::streambuf & receive_buffer) {
		cout << "Reading file..." << endl;

		boost::system::error_code error;
		std::string data = "";
		std::string fin = "";
		PacketStream packets;
		string ackit = getConstStr(ACK);

	bool loop = true;

	while (loop){
		fin = "";
		data = "";

		if( error && error != boost::asio::error::eof ) {
			cout << "receive failed: " << error.message() << endl;
		}
		else {
			std::istream is(&receive_buffer);
			while(std::getline(is, data)) {
				fin = fin + data;
			}
			//cout << "Curr Fin: " << fin << endl;
			//cout << "Curr data: " << data << endl;

		}

		fin.pop_back(); //remove etx

		cout << "Got: " << fin << endl;
		//cout << "Got a packet :)" << endl;

		try{
			fin = base64_decode(fin);
		} catch (int a){
			cout << "Uh oh!" << endl;
		}

		//cout << "Decoded: " << fin << endl;


		if(fin != "leave"){
			packets.push_back(readPacket(fin));
			send_(socket, ackit);

		} else {
			cout << endl << "Finished up. On to the next thing." << endl;
			loop = false;
			send_(socket, ackit);
		}

	}

    return packets; //return final data
}

std::string pack(tcp_header *head, string *bod) {
	tcp_packet packs;
	tcp_header h;
	h = *head;
	packs.header = h.toJson();
	packs.body = *bod;

	return packs.toJson();
}


//------------------------------------------------------------------------------------Begin Main------------------------------------------------------------------------------------//
int main() {

	boost::asio::io_service io_service;

	cout << "Opening Connection..." << endl;

//socket creation
    tcp::socket socket(io_service);

//connection
    socket.connect( tcp::endpoint( boost::asio::ip::address::from_string(HOST), PORT ));

	boost::asio::streambuf receive_buffer;
    boost::system::error_code error;


	//socket.wait(boost::asio::ip::tcp::socket::wait_read);
	cout << "Getting Server Configuration.." << endl;


	//Load configuration
	string configJson = read_(socket, receive_buffer);


	srv_options server_config;
	server_config = readSrvOp(configJson);

	cout << endl << "Configuration Loaded! Current Config: " << endl << endl << server_config.toString() << endl;


	//read_ returns the json of a packet
	PacketStream packs = fileread_(socket, receive_buffer);

	cout << "Read complete." << endl;

	writeFile(&packs, "client_out");


	cout << endl << "Disconnected." << endl;
    return 0;
}//End main
