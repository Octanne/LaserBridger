#include "LaserStream.h"

using namespace std;

#define TRFT_DELAY 10000

/* Safe Pause System */
//gestion des pauses safe
void LaserStream::startSafe()
{
	lastus = getCurrentus();
}
void LaserStream::safePause(int64_t timePause)//appeler avant les changements pour avoir un timing parfait
{
	int64_t currentus = getCurrentus();
	//entre currentus et lastus, il doit y avoir 'timePause' µs (objectif)
	//le temps déja écoulé :
	int64_t dejaEcoule = currentus - lastus;
	//le temps qu'il reste à passer en sleep (pour atteindre l'objectif)
	int64_t time = timePause - dejaEcoule;
	//s'écrit aussi useconds_t tempsRestant = lastMicrosec + timePause - currentusec;
	//(temps objectif (à atteindre) moins le temps actuel)

	if (time >= 0) {
		//moyenne des temps déja passé
		moyenneSleep = moyenneSleep + dejaEcoule;
		moyenneSleepSize++;
		if (maxSleep < dejaEcoule)
			maxSleep = dejaEcoule;
		if (dejaEcoule < minSleep)
			minSleep = dejaEcoule;
#ifdef __linux__
		usleep((useconds_t)time);
#else
		Sleep(time / 1000);
#endif //__linux__
	}
	else
		std::cout << "le capteur a prit un retard de " << (-time) << " µs (us)" << std::endl;
	lastus = lastus + timePause;//peut s'écire "lastus = currentus + time"
	//et quasiment getCurrentus() (sauf que non à cause des calculs qui retardent)
}
int64_t LaserStream::getCurrentus()
{
	return std::chrono::duration_cast<std::chrono::microseconds>(
		std::chrono::system_clock::now().time_since_epoch()
		).count();
}

/* Threads Functions */
void rebuildMessage(vector<int> msgByteArray)
{
	cout << "Unpacking the message..." << endl;
	string message;
	for (int nbBit = 0; nbBit < int(msgByteArray.size()); nbBit=+12) {
		vector<int> charByteArray;
		for (int bit = nbBit; bit < nbBit + 12; bit++) {
			charByteArray.push_back(msgByteArray.at(bit));
		}
		message += char(toInt(charByteArray));
	}
	cout << message << endl;
}
void receiveData(LaserStream *stream)
{
	while (stream->isRunning() && stream->isSynchroned()) {
		//Prepare packet arrived
		while (digitalRead(stream->canalDown()) == 0) { if (!stream->isRunning())return; } //Check if is Synchroned && if Connection pending
		
		vector<int> msgByteArray;
		vector<int> sizeByteArray;
		
		stream->startSafe();//Trft Delay
		//Receive Packet
		for (int bit = 0; bit < 32; bit++) {
			sizeByteArray.push_back(digitalRead(stream->canalDown()));
			cout << "[MessageSize] RECEIVE a bit of value : " << sizeByteArray.at(bit) << endl;
			stream->safePause(); //Trft Delay
		}
		int sizeMessage = toInt(sizeByteArray);
		cout << "SizeMessage = " << sizeMessage << endl;
		for (int bit = 0; bit < sizeMessage; bit++) {
			msgByteArray.push_back(digitalRead(stream->canalDown()));
			cout << "[MessageData] RECEIVE a bit of value : " << msgByteArray.at(bit) << endl;
			stream->safePause(); //Trft Delay
		}
		thread thMessage(rebuildMessage, msgByteArray);
	}
}
void sendData(LaserStream *stream) 
{
	while (stream->isRunning() && stream->isSynchroned()) {

		//Prepare packet arrived
		stream->startSafe();
		while (stream->getPacketWaitingList().empty()) { if (!stream->isRunning())return; } //Check if is Synchroned && if Connection pending
		digitalWrite(stream->canalUp(), 1);
		stream->safePause(); //Safe Pause
		digitalWrite(stream->canalUp(), 0);

		vector<int> sizeByteArray = stream->getPacketWaitingList().at(0).msgSizeByteArray();
		vector<int> msgByteArray = stream->getPacketWaitingList().at(0).msgByteArray();

		//Send Packet
		for (int bit : sizeByteArray) {
			digitalWrite(stream->canalUp(), bit); //Send bit
			cout << "[MessageSize] SEND a bit of value : " << bit << endl;
			stream->safePause(); //Trft Delay
		}
		cout << "SizeMessage = " << toInt(stream->getPacketWaitingList().at(0).msgSizeByteArray()) << endl;
		for (int bit : msgByteArray) {
			digitalWrite(stream->canalUp(), bit); //Send bit
			cout << "[MessageData] SEND a bit of value : " << bit << endl;
			stream->safePause(); //Trft Delay
		}
		digitalWrite(stream->canalUp(), 0);
		stream->getPacketWaitingList().erase(stream->getPacketWaitingList().begin());
	}
}
void synchronization(LaserStream *stream)
{
	/* Synchronazing System */
	stream->startSafe();//Safe Pause
	digitalWrite(stream->canalUp(), 1);
	while (digitalRead(stream->canalDown()) == 0 && stream->isRunning()) {}
	digitalWrite(stream->canalUp(), 0);
	if (!stream->isRunning())stream->close();
	stream->safePause(); //Safe Pause
	cout << endl << "[Sync] synchronization completed !" << endl;
	/* Synchronazing System */
	/* ReceiveData Thread */
	stream->receiveDataThread = new thread(receiveData, stream);
	/* SendData Thread */
	stream->sendDataThread = new thread(sendData, stream);
}

/* Constructors & Destructors */
LaserStream::LaserStream() {

	//Transmitter & Receiver Setting UP
	this->trftDelay_a = TRFT_DELAY;
	canalDown_a = 27;
	canalUp_a = 29;

	//WiringPi Setting UP
	wiringPiSetup();
	pinMode(canalDown_a, INPUT);
	pinMode(canalUp_a, OUTPUT);

	//STATE Value A Retirer après avoir fait le système de syncro
	isSynchroned_a = true;
	isRunning_a = true;

	/* Syncrhonization Thread */
	cout << "Starting synchronization..." << endl;
	synchronizationThread = new thread(synchronization, this);
}
LaserStream::LaserStream(int sendPin, int receivePin) {

	//Transmitter & Receiver Setting UP
	this->trftDelay_a = TRFT_DELAY;
	canalDown_a = receivePin;
	canalUp_a = sendPin;

	//WiringPi Setting UP
	wiringPiSetup();
	pinMode(canalDown_a, INPUT);
	pinMode(canalUp_a, OUTPUT);

	//STATE Value A Retirer après avoir fait le système de syncro
	isSynchroned_a = true;
	isRunning_a = true;

	/* Syncrhonization Thread */
	cout << "Starting synchronization..." << endl;
	synchronizationThread = new thread(synchronization, this);
}
LaserStream::~LaserStream() {

	//Change STATE to stop any Threads
	isRunning_a = false;
	isSynchroned_a = false;

	//Join Threads
	if (receiveDataThread->joinable())receiveDataThread->join();
	if (sendDataThread->joinable())sendDataThread->join();
	if (synchronizationThread->joinable())synchronizationThread->join();

	//WiringPi Désactivation
	pinMode(canalDown_a, INPUT);
	pinMode(canalUp_a, INPUT);
	digitalWrite(canalUp_a, 0);

	//Destroy Object Behind Pointer
	delete receiveDataThread;
	delete sendDataThread;
	delete synchronizationThread;
}

/* Methods */
/* Close Flux */
void LaserStream::close() {
	//Change STATE to stop any Threads
	isRunning_a = false;
	isSynchroned_a = false;

	//Join Threads
	receiveDataThread->join();
	sendDataThread->join();
	synchronizationThread->join();

	//WiringPi Désactivation
	pinMode(canalDown_a, INPUT);
	pinMode(canalUp_a, INPUT);
	digitalWrite(canalUp_a, 0);
	return;
}
/* Transform in ByteArray */
void toByteArray(int const &value_export, vector<int> &byteArrayExport, int sizeFormat) {
	vector<int> byteArray;
	int value = value_export;
	while (value > 0) {
		byteArray.push_back(value % 2);
		value /= 2;
	}
	if (sizeFormat != 0) {
		int nbZeroToAdd = sizeFormat - byteArray.size();
		if (nbZeroToAdd > 0) {
			for (int &nbr = nbZeroToAdd; nbZeroToAdd > 0; nbr--) {
				byteArray.push_back(0);
			}
		}
		else if (nbZeroToAdd < 0) {
			return;
		}
	}
	for (int nbr = byteArray.size() - 1; nbr >= 0; nbr--) {
		byteArrayExport.push_back(byteArray.at(nbr));
	}
}
/* Transform ByteArray in Int */
int toInt(vector<int> &byteArray) {
	int nb = 0;
	for (int nbr = 0; nbr < int(byteArray.size()); nbr++) {
		if (byteArray.at(nbr)) {
			nb += uint(pow(2, (byteArray.size() - 1 - nbr)));
		}
	}
	return nb;
}
/* Send Message */
bool LaserStream::encodingMessage(string &message) {
	if (isSynchroned() && message.length() > 0) {
		//Create DATA Packet
		vector<int> sizeByteArray;
		vector<int> msgByteArray;
		for (char chr : message) {
			int chrValue = int(chr);
			toByteArray(chrValue, msgByteArray, 12);
		}
		toByteArray(msgByteArray.size(), sizeByteArray, 32);
		//Send into WaitList
		packetWaitingList_a.push_back(LaserStream::PacketMessage(msgByteArray, sizeByteArray));
		return true;

		/*
				//Prepare packet arrived
		digitalWrite(stream->canalUp(), 1); 
		//Send Packet
		std::this_thread::sleep_for(std::chrono::nanoseconds(100)); //5
		for (int bit : sizeArrayByte) {
			digitalWrite(stream->canalUp(), bit); //Send bit
			cout << "[MessageSize] SEND a bit of value : " << bit << endl;
			std::this_thread::sleep_for(std::chrono::nanoseconds(100)); //Trft Delay
		}
		toInt(sizeArrayByte);
		for (int bit : msgArrayByte) {
			digitalWrite(stream->canalUp(), bit); //Send bit
			cout << "[MessageData] SEND a bit of value : " << bit << endl;
			std::this_thread::sleep_for(std::chrono::nanoseconds(100)); //Trft Delay
		}
		digitalWrite(stream->canalUp(), 0);
		*/
	}
	else return false;
}
/* Getters */
const bool& LaserStream::isRunning() {
	return isRunning_a;
}
const bool& LaserStream::isSynchroned() {
	return isSynchroned_a;
}
const int& LaserStream::canalUp() {
	return canalUp_a;
}
const int& LaserStream::canalDown() {
	return canalDown_a;
}
const int& LaserStream::trftDelay() {
	return trftDelay_a;
}
vector<LaserStream::PacketMessage>& LaserStream::getPacketWaitingList() {
	return packetWaitingList_a;
}
/* Operators */
LaserStream& operator<<(LaserStream &flux, string message) {
	flux.encodingMessage(message);
	return flux;
}
/*LaserStream& operator>>(LaserStream &flux, string string) {
	string.append(flux.);
	return flux;
}*/