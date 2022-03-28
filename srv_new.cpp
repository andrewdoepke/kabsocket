#include <iostream>
#include <boost/asio.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <random>


using namespace boost::asio;
using ip::tcp;
using std::string;
using std::cout;
using std::cin;
using std::endl;

const int PORT = 1234; //Simulated port for this program

namespace pt = boost::property_tree;



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

  string toJson(){
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
  
  string toString(){
	  std::string s_port_s = "";
 	  std::string d_port_s = "";
	  std::string seq_num_s = "";
	  std::string ack_num_s = "";
	  std::string offset_s = "";
	  std::string flag_n_s = "";
	  std::string window_s = "";
	  std::string checksum_s = "";
	  
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
	  
	  std::string s = "This packet's Data: \nSender Port: " + s_port_s + " \nDestination Port: " + d_port_s + " \nSequence Number: " + seq_num_s + " \nAcknowledgement Number: " + ack_num_s + " \nData Offset: " + offset_s;
	  s = s + " \nPacket Flag: " + flag_n_s + "(" + getConstStr(flag) + ") " + " \nWindow Size: " + window_s + " \nChecksum Value: " + checksum_s + " \n";
	  
	  return s;
  }

};

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

//Create a tcp header from json
tcp_header readHeader(string hdr){
  tcp_header head;

	head = headData(head, hdr);
	
  return head;
}

/*
//TCP Packet Structure
struct tcp_packet {
	tcp_header header;
	string body;

  string toJson(){
    pt::ptree pkt;
    pkt.put("header", header.toJson());
    pkt.put("body", body);

    std::stringstream ss;
    pt::json_parser::write_json(ss, pkt);

    return ss.str(); //return JSON Packet
  }
};



tcp_packet packData(tcp_packet pack){
	pt::ptree reader;

	pt::read_json(pack, reader);
	
	pack.header = reader.get<tcp_header>("header");
	pack.body = reader.get<string>("body");
	
	return (pack);
}

//create packet from json
tcp_packet readPacket(string pkt){
	tcp_packet p;
	p = packData(p);
	
	cout << "test pack data, header is " << p.header.toJson() << endl;
	return p;
}
*/

string read_(tcp::socket & socket) {
       boost::asio::streambuf buf;
       boost::asio::read_until( socket, buf, "\n" );
       string data = boost::asio::buffer_cast<const char*>(buf.data());
       return data;
}

void send_(tcp::socket & socket, const string& message) {
       const string msg = message + "\n";
       boost::asio::write( socket, boost::asio::buffer(message) );
}

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

//Encode a string in base 64
string b64Encode(string raw){
string encoded = "";

return encoded;
}

//Decode a string from base 64
string b64Decode(string b64){
string decoded = "";

return decoded;
}

/*
//Function to send a file to a given connection after being accepted.
int sendFile() {

	unsigned int currPack = 0;
}
*/

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


int main(int argc, char** argv) {

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



	bool debug = true; //He might want us to do debug output, but I doubt he wants us to do it this way. Temporary so we can see our server configuration.
	if(debug){
		cout << endl << "Server Options: " << endl;
		cout << "Protocol Type: " << proType << endl;
		cout << "Packet Size: " << packetSize << endl;
		cout << "Timeout Interval: " << timeout << endl;
		cout << "Sliding Window Size: " << slidingWinSize << endl;
		cout << "Sequence Range: " << seqLower << " to " << seqUpper << " inclusive." << endl << endl;
		cout << "Type of Situational Error: " << sitErrorInp << endl;

		if(sitErrorInp == 3) {
			cout << "Packet numbers to be lost: ";
			for(int n : dropPacket){
				cout << n << " ";
			}

			cout << endl;

			cout << "Ack numbers to be lost: ";
			for(int n : loseAck) {
				cout << n << " ";
			}

			cout << endl;
		}

		cout << "-----------------------------------------------------------------------------" << endl << endl;
	}

//------------------Build our Packet Header Object------------------//
tcp_header base_header;
base_header.seq_num = genSeqNum(seqLower, seqUpper); //generate first seq number
base_header.ack_num = 1; //initialize ack num
base_header.offset = 0; //initialize offset
base_header.flag = IGN; //default flag to ignore
base_header.window = 0; //initialize window size. Probably will have to changed
base_header.checksum = 0; //initialize checksum

if(debug) {
  cout << "Header JSON: " << endl << base_header.toJson() << endl;
  
  cout << "Header converted from the JSON: " << endl << readHeader(base_header.toJson()).toString() << endl;
  cout << "-----------------------------------------------------------------------------" << endl << endl;
}

//------------------Stage the File for Transfer------------------//

	//TODO: Take input file as command line args, we can implement this later. testing might be easier with a hardcode.
	//cout << "We will be transferring " << argv[1] << " to any and all clients."

	//https://www.example-code.com/cpp/base64_encode_file.asp

	//load file in..

	//perhaps we encode the file in base-64? Then, each character is 8 bits and can be appended after recieving.

	//split file between packet bodies using packet size, taking in account the header size.

	//make a vector containing all packets to be sent


//------------------Build all Packets with Body------------------//


//------------------start server functionality------------------//

    boost::asio::io_service io_service;

	while(1){ //loop forever. This will probably have to change and work conditionally.

		cout << "Waiting for a connection..." << endl;

		//Create a listener on specified port
		tcp::acceptor acceptor_(io_service, tcp::endpoint(tcp::v4(), PORT ));

		//Create a tcp socket
		tcp::socket socket_(io_service);

		//Wait for a connection and accept when one comes in
		acceptor_.accept(socket_);

		string cli = boost::lexical_cast<string>(socket_.remote_endpoint().address());

		cout << endl << "Connected!" << endl;
		cout << "Client opened a connection from " << cli << ":" << PORT << endl << endl;

		//Read connection confirmation from client...
		string message = read_(socket_);
		cout << "Client says: " << endl << message << endl;

		//TODO: When client is accepted, we should exchange packets as well as a message. We'd want to tell the client which sliding window protocol to use along with header data.
		//Maybe this should just be sent not as a packet but we just send a string of information to the client before starting anything, given that this is kind of a simulation.

		//Now that client is connected, we will call a function that takes the connection and sends the file to it, and receives and ack. We don't want to send just a string, but that might be useful if we encode the packets to strings

		//example send a string as follows
			send_(socket_, "we're done. i can't date u. u broke my heart :/");
			cout << "Responded to the client." << endl;


		//TODO: Create function and call it to send our packet and whatnot.
		//sendFile();

		//TODO: Maybe add threads for ex credit. it honestly might not be too hard but

		cout << "Finished processing this one." << endl << endl;

	} //end while. we want to keep this server running until killed server-side. built to serve.

	return 0;
}
