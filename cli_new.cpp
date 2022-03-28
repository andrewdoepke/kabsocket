#include <iostream>
#include <boost/asio.hpp>

using namespace boost::asio;
using ip::tcp;
using std::string;
using std::cout;
using std::endl;

const int PORT = 1234; //Simulated port for this program
const string HOST = "127.0.0.1";

//--------------Define Flags--------------//
  const uint8_t IGN = 231; //ignore

	const uint8_t SYN = 0;
	const uint8_t ACK = 1;
	const uint8_t FIN = 2;
	const uint8_t RST = 3;
	const uint8_t PSH = 4;
	const uint8_t URG = 5;

	//For sliding window protocol. This will be separate from normal TCP flags.
	const uint8_t GBN = 6;
	const uint8_t SR = 7;
//---------------------------------------//


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
	//especially the buffer, thats what we'd use for this i think. In the server, this is done via functions :)

    if( error && error != boost::asio::error::eof ) {
         cout << "Failed to recieve data. Error: " << error.message() << endl;
    }
    else {
         const char* data = boost::asio::buffer_cast<const char*>(receive_buffer.data());
         cout << endl << "Server Says: " << endl << data << endl;
    }


	//TODO: Read Packet in
	//TODO: Calculate/validate checksum
	//TODO: Send acknowlegement back


	cout << endl << "Disconnected." << endl;
    return 0;
}//End main
