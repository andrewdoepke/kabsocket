#include <iostream>
#include <boost/asio.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

using namespace boost::asio;
using ip::tcp;
using std::string;
using std::cout;
using std::endl;

const int PORT = 1234; //Simulated port for this program
const string HOST = "127.0.0.1";

namespace pt = boost::property_tree;

typedef std::vector<int> IntVec;

const string delim = "\x04\x03\n";

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


string read_(tcp::socket & socket, boost::asio::streambuf & receive_buffer) {

		std::string fin = "";
		//const char* data = "";
		boost::system::error_code error;
		//boost::asio::streambuf receive_buffer;
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
		
	fin.pop_back(); //remove eto
	fin.pop_back(); //remove etx
	
    return fin; //return final data
}

void send_(tcp::socket & socket, const string& message) {
	const string msg = message + delim;
	boost::system::error_code error;
    boost::asio::write( socket, boost::asio::buffer(msg), error );
	
    if( !error ) {
       cout << "Sent!" << endl;
    }
    else {
       cout << "send failed: " << error.message() << endl;
    }
	
       //const string msg = message + "|";
       //boost::asio::write( socket, boost::asio::buffer(message) );
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

// request/message from client
    const string msg = "hey its me, i miss u...\n";
    boost::system::error_code error;
	
	send_(socket, msg);
	
	
// getting a response from the server

    //boost::asio::streambuf receive_buffer; //create a buffer to read in data
	
	//socket.wait(boost::asio::ip::tcp::socket::wait_read);
    string message = read_(socket, receive_buffer);
	
	//string message = read_(socket);
	//boost::asio::read(socket, receive_buffer, boost::asio::transfer_all(), error); //Read everything from incoming data
	
	cout << "Server sent: " << message << endl;
	
	//Good example of recieving data. We prolly use most of it:
	//especially the buffer, thats what we'd use for this i think. In the server, this is done via functions :)

	
	//socket.wait(boost::asio::ip::tcp::socket::wait_read);
	cout << "Getting Server Configuration.." << endl;
	
	//boost::asio::read(socket, receive_buffer, boost::asio::transfer_all(), error); //Read everything from incoming data
	
	
	//Load configuration
	string configJson = read_(socket, receive_buffer);
	
	//cout << endl << "Config JSON: " << endl << configJson << endl;
	
	
	srv_options server_config;
	server_config = readSrvOp(configJson);
	
	cout << endl << "Configuration Loaded! Current Config: " << endl << endl << server_config.toString() << endl;

	
	//TODO: Read Packet in
	//TODO: Calculate/validate checksum
	//TODO: Send acknowlegement back


	cout << endl << "Disconnected." << endl;
    return 0;
}//End main
