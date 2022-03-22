#include <iostream>  
#include <boost/asio.hpp>  
  
using namespace boost::asio;  
using ip::tcp;  
using std::string;  
using std::cout;  
using std::endl;

#define PORT 1234
#define HOST "127.0.0.1"

int main() {  
	boost::asio::io_service io_service;  

	cout << "Opening Connection..." << endl;

//socket creation  
    tcp::socket socket(io_service);  
  
//connection  
    socket.connect( tcp::endpoint( boost::asio::ip::address::from_string(HOST), PORT ));  
  
// request/message from client  
    const string msg = "hey its me, i miss u...\n";  
    boost::system::error_code error;  
    boost::asio::write( socket, boost::asio::buffer(msg), error );  
    if( !error ) {  
         cout << "Client paired successfully!" << endl;  
    }  
    else {  
         cout << "Could not connect!! Error: " << error.message() << endl;  
    }  
  
// getting a response from the server  

    boost::asio::streambuf receive_buffer; //create a buffer to read in data
    boost::asio::read(socket, receive_buffer, boost::asio::transfer_all(), error); //Read everything from incoming data
	
	//Good example of recieving data. We prolly use most of it:
	
    if( error && error != boost::asio::error::eof ) {  
         cout << "Failed to recieve data. Error: " << error.message() << endl;  
    }  
    else {  
         const char* data = boost::asio::buffer_cast<const char*>(receive_buffer.data());  
         cout << data << endl;  
    }
	
	
	//TODO: Read Packet in
	//TODO: Calculate/validate checksum
	//TODO: Send acknowlegement back
	
    return 0;  
}//End main