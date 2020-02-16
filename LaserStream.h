#ifndef DEF_LASERSTREAM
#define DEF_LASERSTREAM

#include <iostream>
#include <vector>
#include <cmath>
#include <string>
#include <thread>
#include <chrono>
#include <unistd.h>
#include <wiringPi.h>

class LaserStream
{
public:
	struct PacketMessage {
	protected: 
		std::vector<int> msgByteArray_a;
		std::vector<int> msgSizeByteArray_a;

	public:
		PacketMessage(std::vector<int> msgByteArray, std::vector<int> msgSizeByteArray) {
			msgByteArray_a = msgByteArray;
			msgSizeByteArray_a = msgSizeByteArray;
		}
		~PacketMessage() {

		}
		std::vector<int>& msgByteArray() { return msgByteArray_a; }
		std::vector<int>& msgSizeByteArray() { return msgSizeByteArray_a; }
	};

	//Constructors & Destructor
	LaserStream(int sendPin, int receivePin);
	LaserStream();
	~LaserStream();

	//Methods
	bool encodingMessage(std::string &message);
	void close();
	std::vector<PacketMessage>& getPacketWaitingList();

	//Transmitter & Receiver
	const bool& isSynchroned();
	const bool& isRunning();
	const int& canalDown();
	const int& canalUp();
	const int& trftDelay();

	std::thread *receiveDataThread;
	std::thread *sendDataThread;
	std::thread *synchronizationThread;

	//Safe Pause
	static int64_t safeTime() { return 50 * 1000; };//µs, temps de pause pour les capteurs/lasers
	void startSafe();//initialise le chrono pour le safePause
	void safePause(int64_t timePause = safeTime());//fait une pause et gère le temps écoulé dans le code
	static int64_t getCurrentus();

	int64_t moyenneSleep = 0;
	int moyenneSleepSize = 0;
	int64_t maxSleep = 0;
	int64_t minSleep = getCurrentus();
	int64_t lastus = getCurrentus();//derniere execution du safePause, faire un startSafe() pour avoir une valeur sûr.
private:
	int trftDelay_a;
	int canalDown_a, canalUp_a;
	bool isSynchroned_a;
	bool isRunning_a;

	std::vector<PacketMessage> packetWaitingList_a;
};
LaserStream& operator<<(LaserStream &flux, std::string message);
//LaserStream& operator>>(LaserStream &flux, std::string message);
void toByteArray(int const &value, std::vector<int> &arrayByteExport, int sizeFormat = 0);
int toInt(std::vector<int> &byteArray);

#endif