#include <iostream>  
#include <boost/asio.hpp>
#include<boost/algorithm/string.hpp>

  
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

string trim(string input){
	boost::trim_right(input);
	boost::trim_left(input);
	return input;
}


int main() {  
    boost::asio::io_service io_service;

	int proType = 0;
	int packetSize = 0;
	int timeout = -1; //set to -1 to allow another option on 0
	int slidingWinSize = 0;
	int seqLower = 0;
	int seqUpper = 0;
	
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
		cout << "Please enter sliding window size: " << endl;
		cin >> inp;
		if(readIsInt(inp)){//parse the int
			slidingWinSize = stoi(inp);
		}
		if(slidingWinSize < 1 || !readIsInt(inp)){
			cout << "Error! Invalid input. Please try again or CTR-C to quit." << endl;
		}
	}
	
//Range of sequence numbers
	while(seqLower < 1){
		cout << "Please enter the lower range of sequence numbers: " << endl;
		cin >> inp;
		if(readIsInt(inp)){//parse the int
			seqLower = stoi(inp);
		}
		if(seqLower < 1 || !readIsInt(inp)){
			cout << "Error! Invalid input. Please try again or CTR-C to quit." << endl;
			seqLower = 0;
		}
	}
	
	while(seqUpper < 1){
		cout << "Please enter the upper range of sequence numbers: " << endl;
		cin >> inp;
		if(readIsInt(inp)){//parse the int
			seqUpper = stoi(inp);
		}
		if(seqUpper < 1 || !readIsInt(inp) || seqUpper < seqLower){
			cout << "Error! Invalid input. Please try again or CTR-C to quit." << endl;
			
			if(seqUpper < seqLower){
				cout << "\nPlease enter something greater or equal to " << seqLower << endl << endl;
			}
			seqUpper = 0;
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
		cout << endl;
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
	}
	
//------------------Build our Packet Header Struct Object------------------//
	
	
//------------------Stage the File for Transfer------------------//
	//load file in..
	
	//split file between packet bodies using packet size, taking in account the header size.

	//make a vector containing all packets to be sent 
	

//------------------start server functionality------------------//

	while(1){ //loop forever. This will probably have to change and work conditionally.
	  
		//Create a listener on specified port
		tcp::acceptor acceptor_(io_service, tcp::endpoint(tcp::v4(), PORT ));  
		  
		//Create a tcp socket
		tcp::socket socket_(io_service);  
		  
		//Wait for a connection and accept when one comes in
		acceptor_.accept(socket_);  
		  
		//Read connection confirmation from client...
		string message = read_(socket_); 
		cout << message << endl;  
		  
		//Now that client is connected, we will call a function that takes the connection and sends the file to it, and recieves and ack. We don't want to send just a string, but that might be useful if we encode the packets to strings
		
		//example send a string as follows	
			send_(socket_, "we're done. i can't date u. u broke my heart :/");  
			cout << "Responded to the client." << endl;

			
		//TODO: Create function and call it to send our packet and whatnot.
		
			  
		//TODO: Maybe add threads for ex credit. it honestly might not be too hard but
		
		cout << "Finished processing this one. Listening...";
		
	} //end while. we want to keep this server running until killed server-side. built to serve.
	
	return 0;  
}