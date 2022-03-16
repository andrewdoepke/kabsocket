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

int main() {  
      boost::asio::io_service io_service;
	  
//------------------user input------------------//

int proType;
int packetSize;
int timeout;
int slidingWinSize;
int seqLower;
int seqUpper;
int sitError;

//GBN or SR?
	cout << "Would you like to use GBN or SR protocol? 1 for GBN, 2 for SR, anything else to exit."
	cin >> proType;
	
//Packet size

	cin >> packetSize;
	
//Timeout interval (user-specified or ping-calculated)

	cin >> timeout;
	
//Sliding window size

	cin >> slidingWinSize;
	
//Range of sequence numbers

	cin >> seqLower;
	
	cin >> seqUpper;
//Situational errors (none, randomly generated, or user-specified, i.e., drop packets 2, 4, 5, lose acks 11, etc.)

	cin >> sitError;
	  

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