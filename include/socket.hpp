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
 *  GNU Affero General Public License v3 or later
 *
 *  Copyright (C) 2007-2021 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#pragma once
#include <memory>

// C Includes
#include <zlib.h>
#include <netinet/in.h>
#include <arpa/telnet.h>   // IAC, WILL/WONT/DO, SB/SE, TELOPT_*; needed by the telnet:: byte arrays below

// C++ Includes
#include <cstddef>
#include <list>
#include <span>
#include <map>
#include <queue>
#include <set>
#include <vector>
#include <string>
#include <string_view>
#include <sstream>
#include <deque>
#include <array>
#include <fmt/format.h>
#include <nlohmann/json_fwd.hpp>

// standalone asio (ASIO_STANDALONE is supplied by the asio::asio interface target via Crow)
#ifndef ASIO_STANDALONE
#define ASIO_STANDALONE
#endif
#include <asio/ip/tcp.hpp>

#include "msdp.hpp"                                 // for ReportedMsdpVariable

// Defines needed

#define NAWS TELOPT_NAWS
#define TTYPE TELOPT_TTYPE
#define MSDP TELOPT_MSDP
#define CHARSET TELOPT_CHARSET

extern long InBytes;
extern long UnCompressedBytes;
extern long OutBytes;

class Player;
class Account;

typedef struct _xmlNode xmlNode;
typedef xmlNode *xmlNodePtr;


namespace telnet {
    #define TELOPT_CHARSET      42
    #define TELOPT_MSDP         69
    #define TELOPT_MSSP         70
    #define TELOPT_COMPRESS2    86
    #define TELOPT_MSP          90
    #define TELOPT_MXP          91
    #define TELOPT_GMCP         201

    // MTTS capability bits (https://tintin.mudhalla.net/protocols/mtts)
    #define MTTS_ANSI           1
    #define MTTS_VT100          2
    #define MTTS_UTF8           4
    #define MTTS_256COLOR       8
    #define MTTS_MOUSE          16
    #define MTTS_COLORPALETTE   32
    #define MTTS_SCREENREADER   64
    #define MTTS_PROXY          128
    #define MTTS_TRUECOLOR      256
    #define MTTS_MNES           512
    #define MTTS_MSLP           1024
    #define MTTS_SSL            2048

    #define SEND                1
    #define ACCEPTED            2
    #define REJECTED            3

    #define MSSP_VAR            1
    #define MSSP_VAL            2

    #define MSDP_VAR            1
    #define MSDP_VAL            2
    #define MSDP_TABLE_OPEN     3
    #define MSDP_TABLE_CLOSE    4
    #define MSDP_ARRAY_OPEN     5
    #define MSDP_ARRAY_CLOSE    6
    #define MAX_MSDP_SIZE       100

    #define UNICODE_MALE        9794
    #define UNICODE_FEMALE      9792
    #define UNICODE_NEUTER      9791

    #define MXP_BEG             "\x16"
    #define CH_MXP_BEG          '\x16'
    #define MXP_END             "\x04"
    #define CH_MXP_END          '\x04'
    #define MXP_AMP             "\x06"
    #define CH_MXP_AMP          '\x06'

    #define GO_AHEAD            "\x01"
    #define CH_GO_AHEAD         '\x01'

    #define MXP_SECURE_OPEN "\033[1z"
    #define MXP_LOCK_CLOSE "\033[7z"

    constexpr int TELNET_OPT_CMD_LEN = 3;  // IAC <WILL|WONT|DO> <opt>
    constexpr int TELNET_CMD_LEN     = 2;  // IAC <cmd>
    constexpr int MCCP_V2            = 2;  // opts.mccp value when MCCP2 is active


    inline constexpr unsigned char will_msdp[] = { IAC, WILL, TELOPT_MSDP, '\0' }; // Mud Server Data Protocol support
    inline constexpr unsigned char wont_msdp[] = { IAC, WONT, TELOPT_MSDP, '\0' }; // Stop MSDP support

    inline constexpr unsigned char will_gmcp[] = { IAC, WILL, TELOPT_GMCP, '\0' }; // Generic Mud Communication Protocol
    inline constexpr unsigned char wont_gmcp[] = { IAC, WONT, TELOPT_GMCP, '\0' }; // Stop GMCP support

    inline constexpr unsigned char will_mxp[] = { IAC, WILL, TELOPT_MXP, '\0' }; // MXP Support
    inline constexpr unsigned char start_mxp[] = { IAC, SB, TELOPT_MXP, IAC, SE, '\0' }; // Start MXP string

    inline constexpr unsigned char will_comp2[] = { IAC, WILL, TELOPT_COMPRESS2, '\0' }; // MCCP V2 support
    inline constexpr unsigned char start_mccp2[] = { IAC, SB, TELOPT_COMPRESS2, IAC, SE, '\0' }; // Start compress2

    inline constexpr unsigned char will_echo[] = { IAC, WILL, TELOPT_ECHO, '\0' }; // IAC WILL ECHO (server echoes -> client masks input)
    inline constexpr unsigned char wont_echo[] = { IAC, WONT, TELOPT_ECHO, '\0' }; // IAC WONT ECHO (return echo to the client)

    inline constexpr unsigned char will_eor[] = { IAC, WILL, TELOPT_EOR, '\0' }; // EOR after every prompt

    inline constexpr unsigned char will_msp[] = { IAC, WILL, TELOPT_MSP, '\0' }; // MSP Support
    inline constexpr unsigned char wont_msp[] = { IAC, WONT, TELOPT_MSP, '\0' }; // Stop MSP support

    inline constexpr unsigned char will_mssp[] = { IAC, WILL, TELOPT_MSSP, '\0' }; // MSSP Support
    inline constexpr unsigned char sb_mssp_start[] = { IAC, SB, TELOPT_MSSP, '\0' }; // Start MSSP String
    inline constexpr unsigned char sb_mssp_end[] = { IAC, SE, '\0' }; // End MSSP String
    inline constexpr unsigned char mssp_val[] = { MSSP_VAL, '\0' }; // MSSP value marker
    inline constexpr unsigned char mssp_var[] = { MSSP_VAR, '\0' }; // MSSP variable marker

    inline constexpr unsigned char do_ttype[] = { IAC, DO, TELOPT_TTYPE, '\0' }; // Terminal type negotiation
    inline constexpr unsigned char wont_ttype[] = { IAC, WONT, TELOPT_TTYPE, '\0' }; // Refuse terminal type
    inline constexpr unsigned char query_ttype[] = { IAC, SB, TELOPT_TTYPE, TELQUAL_SEND, IAC, SE, '\0' }; // Begin terminal type subnegotiations

    inline constexpr unsigned char do_naws[] = { IAC, DO, TELOPT_NAWS, '\0' }; // Window size negotiation NAWS

    inline constexpr unsigned char do_charset[] = { IAC, DO, TELOPT_CHARSET, '\0' }; // Charset negotiation
    inline constexpr unsigned char charset_utf8[] = { IAC, SB, TELOPT_CHARSET, 1, ' ', 'U', 'T', 'F', '-', '8', IAC, SE, '\0' }; // Negotiate UTF-8

    inline constexpr unsigned char do_new_environ[] = { IAC, DO, TELOPT_NEW_ENVIRON, '\0' }; // DO NEW-ENVIRON (opt 39)
    inline constexpr unsigned char sb_new_environ_send[] = { IAC, SB, TELOPT_NEW_ENVIRON, TELQUAL_SEND, IAC, SE, '\0' }; // SB NEW-ENVIRON SEND (request all vars)

    inline constexpr unsigned char eor_str[] = { IAC, EOR, '\0' }; // IAC EOR end-of-prompt marker
    inline constexpr unsigned char ga_str[] = { IAC, GA, '\0' }; // IAC GA end-of-prompt marker

    long parseMtts(std::string_view ttype);
    std::string mttsCaps(long bits);
    std::map<std::string, std::string> decodeNewEnviron(const std::vector<unsigned char>& sb);


    // For MCCP
    void *zlib_alloc(void *opaque, unsigned int items, unsigned int size);
    void zlib_free(void *opaque, void *address);

    std::string escapeIAC(std::string_view in);
    std::string unescapeIAC(std::string_view in);
    std::string subnegotiate(unsigned char telopt, std::string_view payload, bool escapePayload = true);
    std::string promptGoAhead(bool eor, bool dumb);
    std::string buildMsspPayload(int players, long startTime, short port, std::size_t numClasses, unsigned short raceCount, std::size_t numSkills);
}

class Socket : public std::enable_shared_from_this<Socket> {
    friend class Server;
    struct Host {
        std::string hostName;
        std::string ip;
    };
    struct Term {
        int rows;
        int cols;
        std::string type;
        std::string firstType;
        std::string lastType;
        std::string version;
    };
    struct SockOptions {
        bool            dumb; // Dumb client, don't do telnet negotiations
        int             color;
        bool            xterm256;
        int             mccp;
        bool            mxp;
        bool            mxpClientSecure;
        unsigned char   lastColor;
        bool            msdp;
        bool            gmcp;
        bool            eor;
        bool            msp;
        bool            compressing;
        bool            naws;
        bool            charset;
        bool            utf8;
    };

private:
    static int numSockets;
    ssize_t writeInternal(std::string_view bytes, bool pSpy, bool process);

public:
    enum class TelnetState {
        NEG_NONE,
        NEG_IAC,
        NEG_WILL,
        NEG_WONT,
        NEG_DO,
        NEG_DONT,

        NEG_SB,
        NEG_START_NAWS,
        NEG_SB_NAWS_COL_HIGH,
        NEG_SB_NAWS_COL_LOW,
        NEG_SB_NAWS_ROW_HIGH,
        NEG_SB_NAWS_ROW_LOW,
        NEG_END_NAWS,

        NEG_SB_TTYPE,
        NEG_SB_TTYPE_END,

        NEG_SB_MSDP,
        NEG_SB_MSDP_END,

        NEG_SB_GMCP,
        NEG_SB_GMCP_END,

        NEG_SB_NEW_ENVIRON,
        NEG_SB_NEW_ENVIRON_END,

        NEG_SB_CHARSET,
        NEG_SB_CHARSET_LOOK_FOR_IAC,
        NEG_SB_CHARSET_END,

        NEG_MXP_SECURE,
        NEG_MXP_SECURE_TWO,
        NEG_MXP_SECURE_THREE,
        NEG_MXP_SECURE_FINISH,
        NEG_MXP_SECURE_CONSUME,

        NEG_UNUSED
    };

    // Static Methods
    static void resolveIp(const sockaddr_in &addr, std::string& ip);
    static std::string stripTelnet(std::string_view inStr);
    static bool needsPrompt(std::string_view inStr);
    static std::size_t skipTelnetSeq(std::string_view inStr, std::size_t i);
    void viewFile(const std::string& str, bool shouldPage=false);
    void viewFileReverse(const std::string& str);
    void viewFileReverseReal(const std::string& str);
    void registerPlayer();
public:
    explicit Socket(asio::ip::tcp::socket pSock);   // production: owns the accepted asio socket
    explicit Socket(int pFd);                        // tests: bare fd, synchronous write fallback
    ~Socket();

    void cleanUp();
    void reset();

    void startTelnetNeg();
    void continueTelnetNeg(bool queryTType);
    void setState(int pState, char pFnParam = 1);
    void restoreState();

    void finishLogin();


    ssize_t write(std::string_view text, bool pSpy = true);  // game text: color/MXP/newline + IAC-escaped
    ssize_t writeRaw(std::string_view bytes);                // protocol bytes: verbatim, never spied/processed
    ssize_t writeRaw(const unsigned char* bytes);            // convenience for the NUL-terminated telnet:: arrays
    void echoOff();                                          // mask client input (e.g. passwords)
    void echoOn();                                           // restore client-side echo
    void askFor(const char *str);

    void vprint(const char *fmt, va_list ap);

    void bprint(std::string_view toPrint);
    void bprintPython(const std::string& toPrint);

    template <typename... Args>
    void bprint(std::string_view toPrint, Args &&... args) const {
        return bprint(fmt::format(fmt::runtime(toPrint), std::forward<Args>(args)...));
    }

    void printPaged(std::string_view toPrint);
    void appendPaged(std::string_view toPrint);

    template <typename... Args>
    void printPaged(std::string_view toPrint, Args &&... args) const {
        return printPaged(fmt::format(fmt::runtime(toPrint), std::forward<Args>(args)...));
    }
    void println(std::string_view toPrint = "");
    void print(const char* format, ...);
    void printColor(const char* format, ...);

    std::string parseForOutput(std::string_view outBuf);
    std::string getColorCode(unsigned char ch);

    int processInput();
    int processOneCommand();

    void reconnect(bool pauseScreen=false);
    void disconnect();
    void showLoginScreen();

    void flush(); // Flush any pending output


    int startCompress(bool silent = false);
    int endCompress();

    int sendMSSP(); // Send MSSP Variables

    bool saveTelopts(xmlNodePtr rootNode);
    bool loadTelopts(xmlNodePtr rootNode);

// End Telopt related

    [[nodiscard]] int getFd() const;
    [[nodiscard]] bool isConnected() const;
    [[nodiscard]] int getState() const;
    [[nodiscard]] std::string_view getIp() const;
    [[nodiscard]] std::string_view getHostname() const;

    void checkLockOut();

    void setHostname(std::string_view pName);
    void setIp(std::string_view pIp);

    [[nodiscard]] bool hasOutput() const;
    [[nodiscard]] bool hasCommand() const;

    [[nodiscard]] long getIdle() const;
    [[nodiscard]] int mccpEnabled() const;
    [[nodiscard]] bool mxpEnabled() const;
    [[nodiscard]] bool getMxpClientSecure() const;
    [[nodiscard]] bool msdpEnabled() const;
    [[nodiscard]] bool gmcpEnabled() const;
    [[nodiscard]] bool gmcpSupports(const std::string& pkg) const;
    [[nodiscard]] bool canForce() const;
    [[nodiscard]] bool eorEnabled() const;
    [[nodiscard]] bool isDumbClient() const;
    [[nodiscard]] bool mspEnabled() const;
    [[nodiscard]] bool nawsEnabled() const;
    [[nodiscard]] bool charsetEnabled() const;
    [[nodiscard]] bool utf8Enabled() const;
    [[nodiscard]] long getMtts() const;
    [[nodiscard]] const std::map<std::string, std::string>& getClientEnv() const;

    [[nodiscard]] std::string getTermType() const;
    [[nodiscard]] std::string getClientVersion() const;
    [[nodiscard]] int getColorOpt() const;
    [[nodiscard]] int getTermCols() const;
    [[nodiscard]] int getTermRows() const;

    void setColorOpt(int opt);

    [[nodiscard]] bool hasPlayer() const;
    [[nodiscard]] std::shared_ptr<Player> getPlayer() const;
    void setPlayer(std::shared_ptr<Player> ply);
    void clearPlayer();

    // Account methods
    [[nodiscard]] bool hasAccount() const;
    [[nodiscard]] std::shared_ptr<Account> getAccount() const;
    [[nodiscard]] std::string getAccountName() const;
    [[nodiscard]] std::shared_ptr<Account> getSessionAccount() const;
    [[nodiscard]] std::string getSessionAccountName() const;
    void setAccount(const std::shared_ptr<Account>& acc);
    void clearAccount();

    void clearSpying();
    void clearSpiedOn();
    void setSpying(const std::shared_ptr<Socket>& sock);
    void removeSpy(Socket *sock);
    void addSpy(const std::shared_ptr<Socket>& sock);

    // MXP Support
    void clearMxpClientSecure();
    void defineMxp();

    // MSDP Support Functions
    ReportedMsdpVariable *getReportedMsdpVariable(const std::string &value);
    bool msdpSendPair(std::string_view variable, std::string_view value);
    void msdpSendList(std::string_view variable, const std::vector<std::string>& values);
    void msdpClearReporting();
    std::string getMsdpReporting();
    std::string getGmcpPackages();
    bool gmcpSend(std::string_view package, const nlohmann::json& body);

protected:
    // asio I/O seams (protected so the io tests can drive them directly)
    void startRead();                  // arm an async read on the asio socket
    void resumeRead();                 // re-arm reads paused by input backpressure, once drained
    void doWrite();                    // drive the async_write queue (one write in flight)
    void enqueue(std::string bytes);   // queue outbound bytes and kick the writer
    void drainAndClose();              // best-effort flush of the queue, then close

    // Telopt related
    void decodeBytes(std::span<const unsigned char> data, std::string& out); // telnet FSM, no I/O
    void extractCommands(std::string decoded);                               // CR/LF + backspace + line split
    bool negotiate(unsigned char ch);
    //bool subNegotiate(unsigned char ch);
    bool handleNaws(int& colRow, unsigned char chr, bool high);
    ssize_t processCompressed();

    bool parseMXPSecure();

    void applyMtts(long bits);
    bool parseNewEnviron();

    // MSDP Support Functions
    bool parseMsdp();
    bool processMsdpVarVal(const std::string &variable, const std::string &value);
    bool msdpSend(const std::string &variable);
    bool msdpList(const std::string &value);
    std::vector<std::string> msdpListValues(const std::string &which, std::string &label);
    ReportedMsdpVariable* msdpReport(const std::string &value);
    bool msdpReset(std::string& value);
    bool msdpUnReport(const std::string &value);

    // GMCP Support Functions
    bool parseGmcp();
    bool gmcpSendPackage(const std::string& package);
    nlohmann::json gmcpRoomInfo();
    nlohmann::json gmcpCharGroup();
    nlohmann::json gmcpRoomPlayers();
    nlohmann::json gmcpSkillGroups();
    nlohmann::json gmcpSkillList(const std::string& group);
    nlohmann::json gmcpEffectList(bool defences);
    nlohmann::json gmcpItemsList(const std::string& location);
    void enableGmcpPackage(const std::string& token);
    void gmcpMsdpHandle(const nlohmann::json& body);
    void gmcpMsdpList(const std::string& which);
    void gmcpMsdpSendNow(const std::vector<std::string>& vars);

// TODO - Retool so they can be moved to protected
public:
    char tempstr[4][256]{};
    std::string tempbstr;

    int getParam();
    void setParam(int newParam);

protected:
    std::unique_ptr<asio::ip::tcp::socket> sock;
    int         fd;
    Host        host;
    bool        dnsDone{};
    Term        term;
    SockOptions opts{};

    int         lastState{};
    int         connState{};

    TelnetState tState{};
    bool        oneIAC{};
    bool        watchBrokenClient{};
    long        mtts{};             // MTTS capability bitvector (TTYPE/MNES)

    std::stringstream output;
    std::deque<std::string> writeQueue;        // outbound bytes awaiting async_write
    size_t                  queuedBytes{};     // running total of writeQueue sizes (backlog cap)
    bool                    writeInFlight{};   // true while an async_write is outstanding
    bool                    readPaused{};      // reads suspended while the input queue is full
    std::array<unsigned char, 1024> readBuf{}; // scratch for async_read_some

    std::queue<std::string> input;      // Processed Input buffer

    // IAC buffer, we make it a vector so that it will handle NUL bytes and other characters and still report the correct size()/length()
    std::vector<unsigned char>  cmdInBuf;
    std::string     inBuf;              // Input Buffer
    std::string     inLast;             // Last command

    bool registered{};
    bool cleanedUp{};                  // teardown runs once; ~Socket re-entry is a no-op
    std::shared_ptr<Player>     myPlayer{};
    std::string                 currentAccountName{};  // Account name for this socket


// For MCCP
    struct ZStreamDeleter { void operator()(z_stream* z) const noexcept { deflateEnd(z); delete z; } };
    std::vector<char>                          outCompressBuf;
    std::unique_ptr<z_stream, ZStreamDeleter>  outCompress;     // null unless compressing

// Old items from IOBUF that we might keep
    using CmdFn = void(*)(const std::shared_ptr<Socket>&, const std::string&);
    CmdFn       fn{};
    char        fnparam{};
    char        commands{};
    std::weak_ptr<Socket> spyingOn{};      // Socket we are spying on
    std::list<std::weak_ptr<Socket>> spying;    // Sockets spying on us
    std::map<std::string, ReportedMsdpVariable> msdpReporting;
    std::set<std::string> gmcpStdPackages;          // channel-2 packages from Core.Supports
    std::set<std::string> gmcpMsdpVars;             // channel-1 vars REPORTed via the MSDP package
    std::map<std::string, std::string> clientEnv;   // NEW-ENVIRON/MNES client vars
// TEMP
public:
    long        ltime{};
    char        intrpt{};

private:
    std::deque<std::string> pagerOutput;

public:
    static constexpr int COMPRESSED_OUTBUF_SIZE = 8192;

public:
    static int getNumSockets();
    void handlePaging(const std::string &inStr);
    bool hasPagerOutput();
    int getMaxPages() const;
    void donePaging();

private:
    int paged{};
    void sendPages(int numPages);

};


// Other socket related prototypes
int nonBlock(int pFd);
