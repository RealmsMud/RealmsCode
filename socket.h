/*
 * socket.h
 *   Stuff to deal with sockets
 *   ____            _
 *  |  _ \ ___  __ _| |_ __ ___  ___
 *  | |_) / _ \/ _` | | '_ ` _ \/ __|
 *  |  _ <  __/ (_| | | | | | | \__ \
 *  |_| \_\___|\__,_|_|_| |_| |_|___/
 *
 * Permission to use, modify and distribute is granted via the
 *  Creative Commons - Attribution - Non Commercial - Share Alike 3.0 License
 *    http://creativecommons.org/licenses/by-nc-sa/3.0/
 *
 * 	Copyright (C) 2007-2012 Jason Mitchell, Randi Mitchell
 * 	   Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#ifndef SOCKET_H_
#define SOCKET_H_

// C Includes
#include <zlib.h>
#include <netinet/in.h>

//class sockaddr_in;
// C++ Includes
#include <queue>
#include <string>
#include <list>

// Mud Includes

// Defines needed
#define TELOPTS
#define TELCMDS
#define NAWS TELOPT_NAWS
#define TTYPE TELOPT_TTYPE
#define ATCP TELOPT_ATCP
#define MSDP TELOPT_MSDP
#define CHARSET TELOPT_CHARSET

extern long InBytes;
extern long UnCompressedBytes;
extern long OutBytes;

class Player;
class ReportedMsdpVariable;

namespace telnet {
	#define TELOPT_CHARSET 		42
	#define TELOPT_MSDP         69
	#define TELOPT_MSSP         70
	#define TELOPT_COMPRESS 	85
	#define TELOPT_COMPRESS2 	86
	#define TELOPT_MSP 			90
	#define TELOPT_MXP 			91
	#define TELOPT_ATCP         200

	#define SEND				1
	#define ACCEPTED            2
	#define REJECTED            3

	#define MSSP_VAR 			1
	#define MSSP_VAL 			2

	#define MSDP_VAR            1
	#define MSDP_VAL            2
	#define MSDP_TABLE_OPEN     3
	#define MSDP_TABLE_CLOSE    4
	#define MSDP_ARRAY_OPEN     5
	#define MSDP_ARRAY_CLOSE    6
	#define MAX_MSDP_SIZE       100

	#define UNICODE_MALE		9794
	#define UNICODE_FEMALE		9792
	#define UNICODE_NEUTER		9791

    #define MXP_BEG             "\x03"
    #define CH_MXP_BEG          '\x03'
    #define MXP_END             "\x04"
    #define CH_MXP_END          '\x04'
    #define MXP_AMP             "\x06"
    #define CH_MXP_AMP          '\x06'

    #define MXP_SECURE_OPEN "\033[1z"
    #define MXP_LOCK_CLOSE "\033[7z"


	extern unsigned const char will_msdp[];		// Mud Server Data Protocol support
	extern unsigned const char wont_msdp[];		// Stop MSDP support

	extern unsigned const char do_atcp[];		// ATCP support
	extern unsigned const char wont_atcp[];		// Stop ATCP support

	extern unsigned const char will_mxp[];   	// MXP Support
	extern unsigned const char start_mxp[];		// Start MPX string

	extern unsigned const char will_comp2[]; 	// MCCP V2 support
	extern unsigned const char will_comp1[]; 	// MCCP V1 support
	extern unsigned const char start_mccp[]; 	// Start compress
	extern unsigned const char start_mccp2[];	// Start compress2

	extern unsigned const char will_echo[];  	// Echo input

	extern unsigned const char will_eor[];   	// EOR After every prompt

	extern unsigned const char will_mssp[];  	// MSSP Support
	extern unsigned const char sb_mssp_start[]; // Start MSSP String
	extern unsigned const char sb_mssp_end[];	// End MSSP String

	extern unsigned const char do_ttype[];   	// Terminal type negotation
	extern unsigned const char query_ttype[];	// Begin terminal type subnegotiations

	extern unsigned const char do_naws[];    	// Window size negotation NAWS

	extern unsigned const char do_charset[];    // Window size negotation NAWS
    extern unsigned const char charset_utf8[];	// Negotiate UTF-8


	extern unsigned const char eor_str[];


	// For MCCP
	void *zlib_alloc(void *opaque, unsigned int items, unsigned int size);
	void zlib_free(void *opaque, void *address);
}

class Socket {
	friend class Server;
	struct Host {
		bstring hostName;
		bstring ip;
	};
	struct Term {
		int rows;
		int cols;
		bstring type;
		bstring lastType;
		bstring version;
	};
	struct SockOptions {
		bool		    dumb; // Dumb client, don't do telnet negotiations
		int             color;
		bool            xterm256;
		int			    mccp;
		bool		    mxp;
		bool            mxpClientSecure;
		unsigned char   lastColor;
		bool		    msdp;
		bool 		    atcp;
		bool		    eor;
		bool		    msp;
		bool		    compressing;
		bool		    naws;
		bool		    charset;
		bool		    UTF8;
	};
public:
	// Static Methods
	static void resolveIp(const sockaddr_in &addr, bstring& ip);
	static bstring stripTelnet(bstring& inStr, int& newLen);

public:
	Socket(int pFd);
	Socket(int pFd, sockaddr_in pAddr, bool &dnsDone);
	~Socket();

	void reset();

	void startTelnetNeg();
	void continueTelnetNeg(bool queryTType);
	void setState(int pState, int pFnParam = 1);
	void restoreState();
	void addToPlayerList();

	void finishLogin();


	int write(bstring toWrite, bool pSpy = true, bool process = true);
	void askFor(const char *str);

	void vprint(const char *fmt, va_list ap);

	void bprint(bstring toPrint);
	void bprintColor(bstring toPrint);
	void bprintNoColor(bstring toPrint);
	void println(bstring toPrint = "");
//	void printPrompt(bstring toPrint);
	void print(const char* format, ...);
	void printColor(const char* format, ...);

	bstring parseForOutput(bstring& outBuf);
	bstring getColorCode(const unsigned char ch);

	int processInput(void);
	int processOneCommand(void);

	void reconnect(bool pauseScreen=false);
	void disconnect();
	void showLoginScreen(bool dnsDone=true);

	void flush(void); // Flush any pending output


	int startCompress(bool silent = false);
	int endCompress();

	int sendMSSP(); // Send MSSP Variables

	bool saveTelopts(xmlNodePtr rootNode);
	bool loadTelopts(xmlNodePtr rootNode);

// End Telopt related

	int getFd(void) const;
	bool isConnected() const;
	int getState(void) const;
	const bstring& getIp(void) const;
	const bstring& getHostname(void) const;

	void checkLockOut(void);

	void setHostname(bstring pName);
	void setIp(bstring pIp);

	bool hasOutput(void) const;
	bool hasCommand(void) const;

//	void login(bstring& cmd);
//	void command(bstring& cmd);
//	void createPlayer(bstring& cmd);
	void ANSI(int color);

	long getIdle() const;
	int getMccp() const;
	bool getMxp() const;
	bool getMxpClientSecure() const;
	bool getMsdp() const;
	bool getAtcp() const;
	bool canForce() const;
	bool getEor() const;
	bool isDumbClient() const;
	bool getMsp() const;
	bool getNaws() const;
	bool getCharset() const;
	bool getUtf8() const;

	bstring getTermType() const;
	int getColorOpt() const;
	int getTermCols() const;
	int getTermRows() const;

    void setColorOpt(int opt);

	Player* getPlayer() const;
	void setPlayer(Player* ply);
	void freePlayer();



	void clearSpying();
	void clearSpiedOn();
	void setSpying(Socket *sock);
	void removeSpy(Socket *sock);
	void addSpy(Socket *sock);

	// MXP Support
	void clearMxpClientSecure();
	void defineMxp();

	// MSDP Support Functions
	void msdpSendPair(bstring variable, bstring value);
	void msdpSendList(bstring variable, bstring value);
	void msdpClearReporting();

protected:
	// Telopt related
	bool negotiate(unsigned char ch);
	//bool subNegotiate(unsigned char ch);
	bool handleNaws(int& colRow, unsigned char& chr, bool high);
	int processCompressed(void); // Mccp

	bool parseMXPSecure();

	// MSDP Support Functions
	bool parseMsdp();
	bool processMsdpVarVal(bstring& variable, bstring& value);
	bool parseAtcp();
	bool msdpSend(bstring value);
	bool msdpSend(ReportedMsdpVariable* reportedVar);
	bool msdpList(bstring& value);
	bool msdpReport(bstring& value);
	bool msdpReset(bstring& value);
	bool msdpUnReport(bstring& value);

	ReportedMsdpVariable *getReportedMsdpVariable(bstring& value);
	bool isReporting(bstring& value);
	//const char MsdpCommandList[] = "LIST REPORT RESET SEND UNREPORT";

// TODO - Retool so they can be moved to protected
public:
	char tempstr[4][256];
	bstring tempbstr;

	int getParam();
	void setParam(int newParam);
protected:
	int			fd;					// File Descriptor of this socket
	Host		host;
	Term		term;
	SockOptions	opts;
	bool inPlayerList;

	int			lastState;
	int			connState;


	int			tState;
	bool		oneIAC;
	bool		watchBrokenClient;

	bstring		output;
	bstring		processed_output;	// Output that has been processed but not fully sent (in the case of EWOULDBLOCK for example)

	std::queue<bstring> input;		// Processed Input buffer

	// IAC buffer, we make it a vector so it will handle NUL bytes and other characters
	// and still report the correct size()/length()
	std::vector<unsigned char>	cmdInBuf;
	bstring		inBuf;				// Input Buffer
	bstring		inLast;				// Last command

	Player*		myPlayer;


// From ply extr struct
	int ansi;
	unsigned long timeout;

// For MCCP
	char		*out_compress_buf;
	z_stream    *out_compress;

// Old items from IOBUF that we might keep

	void		(*fn)(Socket*, bstring);

	char		fnparam;

	char		commands;

	Socket		*spyingOn;		// Socket we are spying on
	std::list<Socket*> spying; 	// Sockets spying on us

	std::map<bstring, ReportedMsdpVariable*> msdpReporting;
// TEMP
public:
	long		ltime;
	char		intrpt;


public:
	static const int COMPRESSED_OUTBUF_SIZE;
	static int NumSockets;
};


// Other socket related prototypes
int nonBlock(int pFd);
int restoreState(Socket* sock);


#endif /*SOCKET_H_*/
