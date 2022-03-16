#include <iostream>  
#include <boost/asio.hpp>  
  
using namespace boost::asio;  
using ip::tcp;  
using std::string;  
using std::cout;
using std::cin; 
using std::endl;

#define PORT 1234

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

bool readIsInt(string input){
   for(int i = 0; i < input.length(); i++){
      if(isdigit(input[i]) == false)
         return false;
   }
   return true;
}

int main() {  
    boost::asio::io_service io_service;

	int proType = 0;
	int packetSize = 0;
	int timeout = 0;
	int slidingWinSize = 0;
	int seqLower = 0;
	int seqUpper = 0;
	char sitErrors;
	int dropPacket[100];
	int tempCount = 0;
	string inp = "";
	
//------------------user input------------------//

//GBN or SR?
	while(proType < 1 || proType > 2){
		cout << "Would you like to use GBN or SR protocol? 1 for GBN, 2 for SR."<<endl;
		cin >> inp;
		if(readIsInt(inp)){//parse the int
			proType = stoi(inp);
		}
		if(proType < 1 || !readIsInt(inp)){
			cout << "Error! Invalid input. Please try again or CTR-C to quit." << endl;
		}
	}
	
//Packet size
	while(packetSize < 1){
		cout << "Please enter packet size (in bytes): "<<endl;
		cin >> inp;
		if(readIsInt(inp)){//parse the int
			packetSize = stoi(inp);
		}
		if(packetSize < 1 || !readIsInt(inp)){
			cout << "Error! Invalid input. Please try again or CTR-C to quit." << endl;
		}
	}
	
//Timeout interval (user-specified or ping-calculated)
	cout << "Please enter timeout interval: ";
	cin >> timeout;
	
//Sliding window size
	cout << "Please enter sliding window size: ";
	cin >> slidingWinSize;
	
//Range of sequence numbers
	cout << "Please enter the lower range of sequence numbers: ";
	cin >> seqLower;
	
	cout << "Please enter the upper range of sequence numbers: ";
	cin >> seqUpper;
//Situational errors (none, randomly generated, or user-specified, i.e., drop packets 2, 4, 5, lose acks 11, etc.)

	//Infinitely prompt for drop packets until done
	cout << "Would you like to drop any packets? (Y/N): ";
	cin >> sitErrors;
	while (sitErrors == 'Y') {
		cout << "Please enter a packet you would like to drop: ";
		cin >> dropPacket[tempCount];
		tempCount++;
		cout << "Would you like to drop any other packets? (Y/N): ";
		cin >> sitErrors;
	}
	tempCount = 0;

	

	  

//------------------start server functionality------------------//

	while(1){ //loop forever. This will probably have to change and work conditionally
	  
		//Create a listener on specified port
			  tcp::acceptor acceptor_(io_service, tcp::endpoint(tcp::v4(), PORT ));  
		  
		//Create a tcp socket
			  tcp::socket socket_(io_service);  
		  
		//Wait for a connection and accept when one comes in
			  acceptor_.accept(socket_);  
		  
		//read operation  
			  string message = read_(socket_);  
			  cout << message << endl;  
		  
		//write operation  
			  send_(socket_, "Hello From Server!");  
			  cout << "Servent sent Hello message to Client!" << endl;  
	}
	
	return 0;  
}