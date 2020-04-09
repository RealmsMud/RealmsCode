/*
 * socket.cpp
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
 *  Copyright (C) 2007-2018 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */


// C++ Includes
#include <iostream>
#include <sstream>

// C Includes
#include <arpa/telnet.h>
#include <fcntl.h>      // Needs: fnctl
#include <netdb.h>      // Needs: gethostbyaddr
#include <sys/socket.h> // Needs: AF_INET
#include <cerrno>
#include <cstdlib>

// Mud Includes
#include "commands.hpp"
#include "config.hpp"
#include "creatures.hpp"
#include "login.hpp"
#include "msdp.hpp"
#include "mud.hpp"
#include "post.hpp"
#include "property.hpp"
#include "security.hpp"
#include "server.hpp"
#include "socket.hpp"
#include "version.hpp"
#include "xml.hpp"

// Static initialization
const int Socket::COMPRESSED_OUTBUF_SIZE = 8192;
int Socket::NumSockets = 0;

enum telnetNegotiation {
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

    NEG_SB_ATCP,
    NEG_SB_ATCP_END,

    NEG_SB_GMCP,
    NEG_SB_GMCP_END,

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

//********************************************************************
//                      telnet namespace
//********************************************************************

namespace telnet {
// MSDP Support
unsigned const char will_msdp[] = { IAC, WILL, TELOPT_MSDP, '\0' };
unsigned const char wont_msdp[] = { IAC, WONT, TELOPT_MSDP, '\0' };

// GMCP Support
unsigned const char will_gmcp[] = { IAC, WILL, TELOPT_GMCP, '\0' };
unsigned const char wont_gmcp[] = { IAC, WONT, TELOPT_GMCP, '\0' };

// ATCP Support
unsigned const char do_atcp[] = { IAC, DO, TELOPT_ATCP, '\0' };
unsigned const char wont_atcp[] = { IAC, WONT, TELOPT_ATCP, '\0' };

// MXP Support
unsigned const char will_mxp[] = { IAC, WILL, TELOPT_MXP, '\0' };
// Start mxp string
unsigned const char start_mxp[] = { IAC, SB, TELOPT_MXP, IAC, SE, '\0' };

// MCCP V2 support
unsigned const char will_comp2[] = { IAC, WILL, TELOPT_COMPRESS2, '\0' };
// MCCP V1 support
unsigned const char will_comp1[] = { IAC, WILL, TELOPT_COMPRESS, '\0' };
// Start string for compress2
unsigned const char start_mccp2[] = { IAC, SB, TELOPT_COMPRESS2, IAC, SE, '\0' };
// start string for compress1
unsigned const char start_mccp[] = { IAC, SB, TELOPT_COMPRESS, WILL, SE, '\0' };

// Echo input
unsigned const char will_echo[] = { IAC, WILL, TELOPT_ECHO, '\0' };

// EOR After every prompt
unsigned const char will_eor[] = { IAC, WILL, TELOPT_EOR, '\0' };

// MSP Support
unsigned const char will_msp[] = { IAC, WILL, TELOPT_MSP, '\0' };
// MSP Stop
unsigned const char wont_msp[] = { IAC, WONT, TELOPT_MSP, '\0' };

// MSSP Support
unsigned const char will_mssp[] = { IAC, WILL, TELOPT_MSSP, '\0' };
// MSSP SB
unsigned const char sb_mssp_start[] = { IAC, SB, TELOPT_MSSP, '\0' };
// MSSP SB stop
unsigned const char sb_mssp_end[] = { IAC, SE, '\0' };

// Terminal type negotation
unsigned const char do_ttype[] = { IAC, DO, TELOPT_TTYPE, '\0' };
unsigned const char wont_ttype[] = { IAC, WONT, TELOPT_TTYPE, '\0' };

// Charset
unsigned const char do_charset[] = { IAC, DO, TELOPT_CHARSET, '\0' };
unsigned const char charset_utf8[] = { IAC, SB, TELOPT_CHARSET, 1, ' ', 'U',
        'T', 'F', '-', '8', IAC, SE, '\0' };

// Start sub negotiation for terminal type
unsigned const char query_ttype[] = { IAC, SB, TELOPT_TTYPE, TELQUAL_SEND, IAC, SE, '\0' };
// Window size negotation NAWS
unsigned const char do_naws[] = { IAC, DO, TELOPT_NAWS, '\0' };

// End of line string
unsigned const char eor_str[] = { IAC, EOR, '\0' };

// MCCP Hooks
void *zlib_alloc(void *opaque, unsigned int items, unsigned int size) {
    return calloc(items, size);
}
void zlib_free(void *opaque, void *address) {
    free(address);
}
}

//--------------------------------------------------------------------
// Constructors, Destructors, etc

//********************************************************************
//                      reset
//********************************************************************

void Socket::reset() {
    fd = -1;

    opts.mccp = 0;
    opts.dumb = true;
    opts.mxp = false;
    opts.msp = false;
    opts.eor = false;
    opts.msdp = false;
    opts.atcp = false;
    opts.charset = false;
    opts.UTF8 = false;
    opts.mxpClientSecure = false;
    opts.color = NO_COLOR;
    opts.xterm256 = false;
    opts.lastColor = '\0';

    opts.compressing = false;
    inPlayerList = false;

    out_compress_buf = nullptr;
    out_compress = nullptr;
    myPlayer = nullptr;

    tState = NEG_NONE;
    oneIAC = watchBrokenClient = false;

    term.type = "dumb";
    term.cols = 82;
    term.rows = 40;

    ltime = time(0);
    intrpt = 0;

    fn = 0;
    timeout = 0;
    ansi = 0;
    tState = NEG_NONE;
    connState = LOGIN_START;
    lastState = LOGIN_START;

    zero(tempstr, sizeof(tempstr));
    inBuf = "";
//  cmdInBuf = "";
    spyingOn = 0;
}

//********************************************************************
//                      Socket
//********************************************************************

Socket::Socket(int pFd) {
    reset();
    fd = pFd;
    NumSockets++;
}

Socket::Socket(int pFd, sockaddr_in pAddr, bool &dnsDone) {
    reset();

    struct linger ling;
    fd = pFd;

    resolveIp(pAddr, host.ip);
    resolveIp(pAddr, host.hostName); // Start off with the hostname as the ip, then do
                                     // an asyncronous lookup

// Make this socket non blocking
    nonBlock(fd);

// Set Linger behavior
    ling.l_onoff = ling.l_linger = 0;
    setsockopt(fd, SOL_SOCKET, SO_LINGER, (char *) &ling,
            sizeof(struct linger));

    NumSockets++;
    std::clog << "Constructing socket (" << fd << ") from " << host.ip
            << " Socket #" << NumSockets << std::endl;

    // If we're running under valgrind, we don't resolve dns.  The child
    // process tends to mess with proper memory leak detection
    if (gServer->getDnsCache(host.ip, host.hostName) || gServer->isValgrind()) {
        dnsDone = true;
    } else {
        dnsDone = false;
        setState(LOGIN_DNS_LOOKUP);
        gServer->startDnsLookup(this, pAddr);
    }

    startTelnetNeg();
}

//********************************************************************
//                      closeFd
//********************************************************************
// Disconnect the underlying file descriptor
void Socket::cleanUp() {
    clearSpying();
    clearSpiedOn();
    msdpClearReporting();

    if (myPlayer) {
        if (myPlayer->fd > -1) {
            myPlayer->save(true);
            myPlayer->uninit();
        }
        freePlayer();
    }
    endCompress();
    if(fd > -1) {
        close(fd);
        fd = -1;
    }

}
//********************************************************************
//                      ~Socket
//********************************************************************

Socket::~Socket() {
    std::clog << "Deconstructing socket , ";
    NumSockets--;
    std::clog << "Num sockets: " << NumSockets << std::endl;
    cleanUp();
}

//********************************************************************
//                      addToPlayerList
//********************************************************************

void Socket::addToPlayerList() {
    inPlayerList = true;
}

//********************************************************************
//                      freePlayer
//********************************************************************

void Socket::freePlayer() {
    if (myPlayer)
        free_crt(myPlayer, inPlayerList);
    myPlayer = 0;
    inPlayerList = false;
}

// End - Constructors, Destructors, etc
//--------------------------------------------------------------------

//********************************************************************
//                      clearSpying
//********************************************************************

void Socket::clearSpying() {
    if (spyingOn) {
        spyingOn->removeSpy(this);
        spyingOn = 0;
    }
}

//********************************************************************
//                      clearSpiedOn
//********************************************************************

void Socket::clearSpiedOn() {
    std::list<Socket*>::iterator it;
    for (it = spying.begin(); it != spying.end(); it++) {
        Socket *sock = *it;
        if (sock)
            sock->setSpying(nullptr);
    }
    spying.clear();
}

//********************************************************************
//                      setSpying
//********************************************************************

void Socket::setSpying(Socket *sock) {
    if (sock)
        clearSpying();
    spyingOn = sock;

    if (sock)
        sock->addSpy(this);
    if (!sock) {
        if (myPlayer)
            myPlayer->clearFlag(P_SPYING);
    }
}

//********************************************************************
//                      removeSpy
//********************************************************************

void Socket::removeSpy(Socket *sock) {
    spying.remove(sock);
    if (myPlayer->getClass() >= sock->myPlayer->getClass())
        sock->printColor("^r%s is no longer observing you.\n",
                sock->myPlayer->getCName());
}

//********************************************************************
//                      addSpy
//********************************************************************

void Socket::addSpy(Socket *sock) {
    spying.push_back(sock);
}

//********************************************************************
//                      disconnect
//********************************************************************
// Set this socket for removal on the next cleanup

void Socket::disconnect() {
    // TODO: Perhaps remove player from the world right here.
    setState(CON_DISCONNECTING);
    flush();
    cleanUp();
}

//********************************************************************
//                      resolveIp
//********************************************************************

void Socket::resolveIp(const sockaddr_in &addr, bstring& ip) {
    std::ostringstream tmp;
    long i = htonl(addr.sin_addr.s_addr);
    tmp << ((i >> 24) & 0xff) << "." << ((i >> 16) & 0xff) << "."
            << ((i >> 8) & 0xff) << "." << (i & 0xff);
    ip = tmp.str();
}

bstring Socket::parseForOutput(bstring& outBuf) {
    int i = 0;
    ssize_t n = outBuf.size();
    std::ostringstream oStr;
    bool inTag = false, inEntity = false;
    unsigned char ch = 0;
    while(i < n) {
        ch = outBuf[i++];
        if(inTag) {
            if(ch == CH_MXP_END) {
                inTag = false;
                if(opts.mxp)
                    oStr << ">" << MXP_LOCK_CLOSE;
            } else if(opts.mxp)
                oStr << ch;

            continue;
        } else if(inEntity) {
            if(opts.mxp)
                oStr << ch;
            if(ch == ';')
                inEntity = false;
            continue;
        } else {
            if(ch == CH_MXP_BEG) {
                inTag = true;
                if(opts.mxp)
                    oStr << MXP_SECURE_OPEN << "<";
                continue;
            } else {
                if(ch == '^') {
                    ch = outBuf[i++];
                    oStr << getColorCode(ch);
                } else if(ch == '\n') {
                    oStr << "\r\n";
                } else {
                    oStr << ch;
                }
                continue;
            }
        }
    }
    return(oStr.str());

}

bool Socket::needsPrompt(bstring& inStr) {
    int i = 0;
    ssize_t n = inStr.size();

    while(i < n) {
        if((unsigned char)inStr[i] == IAC) {
            switch((unsigned char)inStr[i+1]) {
                case WILL:
                case WONT:
                case DO:
                    // Skip 3
                    i+=3;
                    continue;
                case EOR:
                    // Skip 2
                    i+=2;
                    continue;
                case SB:
                    // Skip until we find IAC SE
                    while(i < n) {
                        if((unsigned char)inStr[i] == IAC && (unsigned char)inStr[i+1] == SE) {
                            // Skip two more
                            i += 2;
                            break;
                        }
                        i++;
                    }
                    continue;
            }
        }
        return true;
    }
    return false;
}

bstring Socket::stripTelnet(bstring& inStr) {
    int i = 0;
    ssize_t n = inStr.size();
    std::ostringstream oStr;

    while(i < n) {
        if((unsigned char)inStr[i] == IAC) {
            switch((unsigned char)inStr[i+1]) {
            case WILL:
            case WONT:
            case DO:
                // Skip 3
                i+=3;
                continue;
            case EOR:
                // Skip 2
                i+=2;
                continue;
            case SB:
                // Skip until we find IAC SE
                while(i < n) {
                    if((unsigned char)inStr[i] == IAC && (unsigned char)inStr[i+1] == SE) {
                        // Skip two more
                        i += 2;
                        break;
                    }
                    i++;
                }
                continue;
            }
        }
        oStr << inStr[i++];
    }
    return(oStr.str());
}

//********************************************************************
//                      checkLockOut
//********************************************************************

void Socket::checkLockOut() {
    int lockStatus = gConfig->isLockedOut(this);
    if (lockStatus == 0) {
        askFor("\n\nPlease enter name: ");
        setState(LOGIN_GET_NAME);
    } else if (lockStatus == 2) {
        print("\n\nA password is required to play from your site: ");
        setState(LOGIN_GET_LOCKOUT_PASSWORD);
    } else if (lockStatus == 1) {
        print("\n\nYou are not wanted here. Begone.\n");
        setState(CON_DISCONNECTING);
    }
}

//********************************************************************
//                      startTelnetNeg
//********************************************************************

void Socket::startTelnetNeg() {
    // As noted from KaVir -
    // Some clients (such as GMud) don't properly handle negotiation, and simply
    // display every printable character to the screen.  However TTYPE isn't a
    // printable character, so we negotiate for it first, and only negotiate for
    // other protocols if the client responds with IAC WILL TTYPE or IAC WONT
    // TTYPE.  Thanks go to Donky on MudBytes for the suggestion.

    write(telnet::do_ttype, false);

}
void Socket::continueTelnetNeg(bool queryTType) {
    if (queryTType)
        write(telnet::query_ttype, false);

    // Not a dumb client if we've gotten a response
    opts.dumb = false;
    write(telnet::will_comp2, false);
    write(telnet::will_comp1, false);

    write(telnet::do_naws, false);
//  write(telnet::will_gmcp, false);
    write(telnet::will_msdp, false);
//    write(telnet::do_atcp, false);
    write(telnet::will_mssp, false);
    write(telnet::will_msp, false);
//  write(telnet::do_charset, false);  // Not implemented yet
    write(telnet::will_mxp, false);
    write(telnet::will_eor, false);
}

//********************************************************************
//                      processInput
//********************************************************************

int Socket::processInput() {
    unsigned char tmpBuf[1024];
    ssize_t n;
    ssize_t i = 0;
    bstring tmp = "";

    // Attempt to read from the socket
    n = read(getFd(), tmpBuf, 1023);
    if (n <= 0) {
        if (errno != EWOULDBLOCK)
            return (-1);
        else
            return (0);
    }

    tmp.reserve(n);

    InBytes += n;

    tmpBuf[n] = '\0';
    // If we have any full strings, copy it over to the queue to be interpreted

    // Look for any IAC commands using a finite state machine
    for (i = 0; i < n; i++) {

// For debugging
//      std::clog << "DEBUG:" << (unsigned int)tmpBuf[i] << "'" << (unsigned char)tmpBuf[i] << "'" << "\n";

        // Try to handle zMud, cMud & tintin++ which don't seem to double the IAC for NAWS
        // during my limited testing -JM
        if (oneIAC && tState > NEG_START_NAWS && tState < NEG_END_NAWS && tmpBuf[i] != IAC) {
            // Broken Client
            std::clog << "NAWS: BUG - Broken Client: Non-doubled IAC\n";
            i--;
        }
        if (watchBrokenClient) {
            // If we just finished NAWS with a 255 height...keep an eye out for the next
            // character to be a stray SE
            if (tState == NEG_NONE && (unsigned char) tmpBuf[i] == SE) {
                std::clog << "NAWS: BUG - Stray SE\n";
                // Set the tState to NEG_IAC as it should have been, and carry gracefully on
                tState = NEG_IAC;
            }
            // It should only be the next character, so if we don't find it...don't keep looking for it
            watchBrokenClient = false;
        }

        switch (tState) {
            case NEG_NONE:
                // Expecting an IAC here
                if ((unsigned char) tmpBuf[i] == IAC) {
                    tState = NEG_IAC;
                    break;
                } else if((unsigned char)tmpBuf[i] == '\033') {
                    tState = NEG_MXP_SECURE;
                    break;
                } else {
                    tmp += tmpBuf[i];
                    break;
                }
                break;
            case NEG_MXP_SECURE:
                if(tmpBuf[i] == '[') {
                    tState = NEG_MXP_SECURE_TWO;
                    break;
                } else {
                    tmp += "\033" + bstring(tmpBuf[i]);
                }
                tState = NEG_NONE;
                break;
            case NEG_MXP_SECURE_TWO:
                if(tmpBuf[i] == '1') {
                    tState = NEG_MXP_SECURE_FINISH;
                    break;
                } else {
                    tmp += "\033[" + bstring(tmpBuf[i]);
                }
                tState = NEG_NONE;
                break;
            case NEG_MXP_SECURE_FINISH:
                if(tmpBuf[i] == 'z') {
                    opts.mxpClientSecure = true;
                    tState = NEG_MXP_SECURE_CONSUME;
                    std::clog << "Client secure MXP mode enabled" << std::endl;
                    break;
                } else {
                    tmp += "\033[1" + bstring(tmpBuf[i]);
                }
                tState = NEG_NONE;
                break;
            case NEG_MXP_SECURE_CONSUME:
                if(tmpBuf[i] == '\n') {
                    tState = NEG_NONE;
                    parseMXPSecure();
                } else {
                    cmdInBuf.push_back(tmpBuf[i]);
                }
                break;
            case NEG_IAC:
                switch ((unsigned char) tmpBuf[i]) {
                    case NOP:
                    case IP:
                    case GA:
                        tState = NEG_NONE;
                        break;
                    case WILL:
                        tState = NEG_WILL;
                        break;
                    case WONT:
                        tState = NEG_WONT;
                        break;
                    case DO:
                        tState = NEG_DO;
                        break;
                    case DONT:
                        tState = NEG_DONT;
                        break;
                    case SE:
                        tState = NEG_NONE;
                        break;
                    case SB:
                        tState = NEG_SB;
                        break;
                    case IAC:
                        // Doubled IAC, send along to parser
                        tmp += tmpBuf[i];
                        tState = NEG_NONE;
                        break;
                    default:
                        tState = NEG_NONE;
                        break;
                }
                break;
                // Handle Do and Will
            case NEG_DO:
            case NEG_WILL:
            case NEG_DONT:
            case NEG_WONT:
                negotiate((unsigned char) tmpBuf[i]);
                break;
            case NEG_SB:
                switch ((unsigned char) tmpBuf[i]) {
                    case NAWS:
                        tState = NEG_SB_NAWS_COL_HIGH;
                        break;
                    case TTYPE:
                        tState = NEG_SB_TTYPE;
                        break;
                    case CHARSET:
                        if (getCharset())
                            tState = NEG_SB_CHARSET;
                        else
                            tState = NEG_NONE;
                        break;
                    case MSDP:
                        tState = NEG_SB_MSDP;
                        break;
                    case ATCP:
                        tState = NEG_SB_ATCP;
                        break;
                    case TELOPT_GMCP:
                        tState = NEG_SB_GMCP;
                        break;
                    default:
                        std::clog << "Unknown Sub Negotiation: " << (int)tmpBuf[i] << std::endl;
                        tState = NEG_NONE;
                        break;
                }
                break;
            case NEG_SB_MSDP:
                cmdInBuf.push_back(tmpBuf[i]);
                if (tmpBuf[i] == IAC) {
                    tState = NEG_SB_MSDP_END;
                    break;
                }
                break;
            case NEG_SB_MSDP_END:
                if (tmpBuf[i] == SE) {
                    // We should have a full ATCP command now, let's parse it now
                    parseMsdp();
                    tState = NEG_NONE;
                    break;
                } else {
                    // Not an SE: The last input was an IAC, so push the new input
                    // onto the inbuf and keep going
                    cmdInBuf.push_back(tmpBuf[i]);
                    tState = NEG_SB_MSDP;
                    break;
                }
                break;
            case NEG_SB_GMCP:
                // We don't handle this right now, but ignoring it because Mudlet likes to send it anyway
                if(tmpBuf[i] == IAC) {
                    tState = NEG_SB_GMCP_END;
                    break;
                }
                break;
            case NEG_SB_GMCP_END:
                if (tmpBuf[i] == SE) {
                    // We should have a full GMCP command now, let's ignore it
                    tState = NEG_NONE;
                    break;
                } else {
                    // Not an SE: The last input was an IAC, so keep going
                    cmdInBuf.push_back(tmpBuf[i]);
                    tState = NEG_SB_GMCP;
                    break;
                }
                break;
            case NEG_SB_ATCP:
                if (tmpBuf[i] == IAC) {
                    tState = NEG_SB_ATCP_END;
                    break;
                }
                cmdInBuf.push_back(tmpBuf[i]);
                break;
            case NEG_SB_ATCP_END:
                if (tmpBuf[i] == SE) {
                    // We should have a full ATCP command now, let's parse it now
                    parseAtcp();
                    tState = NEG_NONE;
                    break;
                } else {
                    // Not an SE: The last input was an IAC, so push the new input
                    // onto the inbuf and keep going
                    cmdInBuf.push_back(tmpBuf[i]);
                    tState = NEG_SB_ATCP;
                    break;
                }
                break;
            case NEG_SB_CHARSET:
                // We've only asked for UTF-8, so assume if they respond it's for that
                // and just eat the rest of the input
                //
                // Any other sub-negotiations (such as TTABLE-*) are not handled
                if (tmpBuf[i] == ACCEPTED) {
                    std::clog << "Enabled UTF8" << std::endl;
                    opts.UTF8 = true;
                    tState = NEG_SB_CHARSET_LOOK_FOR_IAC;
                } else if (tmpBuf[i] == REJECTED) {
                    opts.UTF8 = false;
                    tState = NEG_SB_CHARSET_LOOK_FOR_IAC;
                } else {
                    tState = NEG_SB_CHARSET_LOOK_FOR_IAC;
                }
                break;
            case NEG_SB_CHARSET_LOOK_FOR_IAC:
                // Do nothing while we wait for an IAC
                if (tmpBuf[i] == IAC)
                    tState = NEG_SB_CHARSET_END;
                break;
            case NEG_SB_CHARSET_END:
                if (tmpBuf[i] == IAC) {
                    // Double IAC, part of the data
                    tState = NEG_SB_CHARSET_LOOK_FOR_IAC;
                    break;
                } else if (tmpBuf[i] == SE) {
                    // Found what we were looking for
                } else {
                    std::clog << "NEG_SB_CHARSET_END Error: Expected SE, got '" << (int) tmpBuf[i]
                            << "'" << std::endl;
                }
                tState = NEG_NONE;
                break;
            case NEG_SB_TTYPE:
                // Grab the terminal type
                if (tmpBuf[i] == TELQUAL_IS) {
                    term.lastType = term.type;
                    term.type.erase();
                    term.type = "";
                } else if (tmpBuf[i] == IAC) {
                    // Expect a SE next
                    tState = NEG_SB_TTYPE_END;
                    break;
                } else {
                    term.type += tmpBuf[i];
                }
                break;
            case NEG_SB_TTYPE_END:
                if (tmpBuf[i] == SE) {
                    // TODO: Save the first one and once we get the last one, either:
                    // 1) turn off ttype and turn it back on once to get the first reported type
                    // 2) request it one more time to get the first time
                    std::clog << "Found term type: " << term.type << std::endl;
                    // No previous term type
                    // Or the current term type isn't the same as the last
                    if (term.lastType.empty() ||  (term.type != term.lastType)) {
                        term.lastType = "";
                        // Look for 256 color support
                        if (term.type.find("-256color") != bstring::npos) {
                            // Works for tintin++, wintin++ and blowtorch
                            opts.xterm256 = true;
                        }

                        // Request another!
                        write(telnet::query_ttype, false);
                    }

                    if (term.type.find("Mudlet") != bstring::npos and term.type > "Mudlet 1.1") {
                        opts.xterm256 = true;
                    } else if(term.type.equals("EMACS-RINZAI", false) ||
                              term.type.find("DecafMUD") != bstring::npos) {
                        opts.xterm256 = true;
                    }

                } else if (tmpBuf[i] == IAC) {
                    // I doubt this will happen
                    std::clog << "NEG_SB_TTYPE: Found double IAC" << std::endl;
                    term.type += tmpBuf[i];
                    tState = NEG_SB_TTYPE;
                    break;
                } else {
                    std::clog << "NEG_SB_TTYPE_END Error: Expected SE, got '" << (int) tmpBuf[i]
                            << "'" << std::endl;
                }

                tState = NEG_NONE;
                break;
            case NEG_SB_NAWS_COL_HIGH:
                if (handleNaws(term.cols, tmpBuf[i], true))
                    tState = NEG_SB_NAWS_COL_LOW;
                break;
            case NEG_SB_NAWS_COL_LOW:
                if (handleNaws(term.cols, tmpBuf[i], false))
                    tState = NEG_SB_NAWS_ROW_HIGH;
                break;
            case NEG_SB_NAWS_ROW_HIGH:
                if (handleNaws(term.rows, tmpBuf[i], true))
                    tState = NEG_SB_NAWS_ROW_LOW;
                break;
            case NEG_SB_NAWS_ROW_LOW:
                if (handleNaws(term.rows, tmpBuf[i], false)) {
                    std::clog << "New term size: " << term.cols << " x " << term.rows << std::endl;
                    // Some clients (tintin++, cmud, possibly zmud) don't seem to double an IAC(255) when it's
                    // sent as data, if this happens in the cols...we should be able to gracefully catch it
                    // but if it happens in the rows...it'll eat the IAC from IAC SE and cause problems,
                    // so we set the state machine to keep an eye out for a stray SE if the rows were set to 255
                    if (term.rows == 255)
                        watchBrokenClient = true;
                    tState = NEG_NONE;
                }
                break;
            default:
                std::clog << "Unhandled state" << std::endl;
                tState = NEG_NONE;
                break;
        }
    }

    // Handles the screwy windows telnet, and its not that hard for
    // other clients that send \n\r too
    tmp.Replace("\r", "\n");
    inBuf += tmp;

    // handle backspaces
    n = inBuf.getLength();

    for (i = MAX<int>(n - tmp.getLength(), 0); i < (unsigned) n; i++) {
        if (inBuf.getAt(i) == '\b' || inBuf.getAt(i) == 127) {
            if (n < 2) {
                inBuf = "";
                n = 0;
            } else {
                inBuf.Delete(i - 1, 2);
                n -= 2;
                i--;
            }
        }
    }

    bstring::size_type idx = 0;
    while ((idx = inBuf.find("\n", 0)) != bstring::npos) {
        bstring tmpr = inBuf.substr(0, idx); // Don't copy the \n
        idx += 1; // Consume the \n
        if (inBuf[idx] == '\n')
            idx += 1; // Consume the extra \n if applicable

        inBuf.erase(0, idx);
        if(opts.mxpClientSecure) {
            if(inBuf.left(8).equals("<version", false)) {
                std::clog << "Got msxp version\n";
            } else if(inBuf.left(9).equals("<supports", false)) {
                std::clog << "Got msxp supports\n";
            }
        }
        input.push(tmpr);
    }
    ltime = time(0);
    return (0);
}

bool Socket::negotiate(unsigned char ch) {

    switch (ch) {
        case TELOPT_CHARSET:
            if (tState == NEG_WILL) {
                opts.charset = true;
                write(telnet::charset_utf8, false);
                std::clog << "Charset On" << std::endl;
            } else if (tState == NEG_WONT) {
                opts.charset = false;
                std::clog << "Charset Off" << std::endl;
            }
            tState = NEG_NONE;
            break;
        case TELOPT_TTYPE:
            // If we've gotten this far, we're fairly confident they support ANSI color
            // so enable that
            opts.color = ANSI_COLOR;

            // If we get here it's clearly not a dumb terminal, however if
            // dumb is still set, it means we haven't negotiated, so let's negotiate now
            if (opts.dumb) {
                if (tState == NEG_WILL) {
                    // Continue and query the rest of the options, including term type
                    std::clog << "Continuing telnet negotiation\n";
                    continueTelnetNeg(true);
                } else if (tState == NEG_WONT) {
                    // If they respond to something here they know how to negotiate,
                    // so continue and ask for the rest of the options, except term type
                    // which they have just indicated they won't do
                    write(telnet::wont_ttype);
                    continueTelnetNeg(false);

                }
            }
            tState = NEG_NONE;
            break;
        case TELOPT_MXP:
            if (tState == NEG_WILL || tState == NEG_DO) {
                write(telnet::start_mxp);
                // Start off in MXP LOCKED CLOSED
                //TODO: send elements we're using for mxp
                opts.mxp = true;
                std::clog << "Enabled MXP" << std::endl;
                defineMxp();
            } else if (tState == NEG_WONT || tState == NEG_DONT) {
                opts.mxp = false;
                std::clog << "Disabled MXP" << std::endl;
            }
            tState = NEG_NONE;
            break;
        case TELOPT_COMPRESS2:
            if (tState == NEG_WILL || tState == NEG_DO) {
                opts.mccp = 2;
                startCompress();
            } else if (tState == NEG_WONT || tState == NEG_DONT) {
                if (opts.mccp == 2) {
                    opts.mccp = 0;
                    endCompress();
                }
            }
            tState = NEG_NONE;
            break;
        case TELOPT_COMPRESS:
            if (tState == NEG_WILL || tState == NEG_DO) {
                opts.mccp = 1;
                startCompress();
            } else if (tState == NEG_WONT || tState == NEG_DONT) {
                if (opts.mccp == 1) {
                    opts.mccp = 0;
                    endCompress();
                }
            }
            tState = NEG_NONE;
            break;
        case TELOPT_EOR:
            if (tState == NEG_WILL || tState == NEG_DO) {
                opts.eor = true;
                std::clog << "Activating EOR\n";
            } else if (tState == NEG_WONT || tState == NEG_DONT) {
                opts.eor = false;
                std::clog << "Deactivating EOR\n";
            }
            tState = NEG_NONE;
            break;
        case TELOPT_NAWS:
            if (tState == NEG_WILL) {
                opts.naws = true;
            } else {
                opts.naws = false;
            }
            tState = NEG_NONE;
            break;
        case TELOPT_ECHO:
        case TELOPT_NEW_ENVIRON:
            // TODO: Echo/New Environ
            tState = NEG_NONE;
            break;
        case TELOPT_MSSP:
            if (tState == NEG_DO) {
                sendMSSP();
            }
            tState = NEG_NONE;
            break;
        case TELOPT_MSP:
            if (tState == NEG_WILL || tState == NEG_DO) {
                opts.msp = true;
            } else if (tState == NEG_WONT || tState == NEG_DONT) {
                opts.msp = false;
            }

            tState = NEG_NONE;
            break;
        case TELOPT_MSDP:
            if (tState == NEG_DO) {
                opts.msdp = true;
                msdpSend("SERVER_ID");

                std::clog << "Enabled MSDP" << std::endl;
            } else {
                std::clog << "Disabled MSDP" << std::endl;
                opts.msdp = false;
            }
            tState = NEG_NONE;
            break;
        case TELOPT_ATCP:
            if (tState == NEG_WILL || tState == NEG_DO) {
                opts.atcp = true;
                std::clog << "Enabled ATCP" << std::endl;
                msdpSend("SERVER_ID");
    //          msdpSendPair("SERVER_ID", "The Realms of Hell v" VERSION);
            } else {
                std::clog << "Disabled ATCP" << std::endl;
                opts.atcp = false;
            }
            tState = NEG_NONE;
            break;
        default:
            tState = NEG_NONE;
            break;
    }
    return (true);
}

//********************************************************************
//                      handleNaws
//********************************************************************
// Return true if a state should be changed

bool Socket::handleNaws(int& colRow, unsigned char& chr, bool high) {
    // If we get an IAC here, we need a double IAC
    if (chr == IAC) {
        if (!oneIAC) {
//          std::clog << "NAWS: Got ONE IAC, checking for two.\n";
            oneIAC = true;
            return (false);
        } else {
//          std::clog << "NAWS: Got second IAC.\n";
            oneIAC = false;
        }
    } else if (oneIAC && chr != IAC) {
        // Error!
        std::clog << "NAWS: BUG - Expecting a doubled IAC, got " << (unsigned int) chr << "\n";
        oneIAC = false;
    }

    if (high)
        colRow = chr << 8;
    else
        colRow += chr;

    return (true);
}

//********************************************************************
//                      processOneCommand
//********************************************************************
// Aka interpreter

int Socket::processOneCommand(void) {
    bstring cmd = input.front();
    input.pop();

    // Send the command to the people we're spying on
    if (!spying.empty()) {
        std::list<Socket*>::iterator it;
        for (it = spying.begin(); it != spying.end(); it++) {
            Socket *sock = *it;
            if (sock)
                sock->write("[" + cmd + "]\n", false);
        }
    }

    ((void(*)(Socket*, bstring)) (fn))(this, cmd);

    return (1);
}

//********************************************************************
//                      restoreState
//********************************************************************
// Returns a fd to it's previous state

void Socket::restoreState() {
    setState(lastState);
    createPlayer(this, "");
    return;
}

int restoreState(Socket* sock) {
    sock->restoreState();
    return (0);
}

//*********************************************************************
//                      pauseScreen
//*********************************************************************

void pauseScreen(Socket* sock, const bstring& str) {
    if(str.equals("quit"))
        sock->disconnect();
    else
        sock->reconnect();
}

//*********************************************************************
//                      reconnect
//*********************************************************************

void Socket::reconnect(bool pauseScreen) {
    clearSpying();
    clearSpiedOn();
    msdpClearReporting();

    freePlayer();

    if (pauseScreen) {
        setState(LOGIN_PAUSE_SCREEN);
        printColor(
                "\nPress ^W[RETURN]^x to reconnect or type ^Wquit^x to disconnect.\n: ");
    } else {
        setState(LOGIN_GET_NAME);
        showLoginScreen();
    }
}

//*********************************************************************
//                      setState
//*********************************************************************
// Sets a fd's state and changes the interpreter to the appropriate function

void Socket::setState(int pState, int pFnParam) {
    // Only store the last state if we're changing states, used mainly in the viewing file states
    if (pState != connState)
        lastState = connState;
    connState = pState;

    if (pState == LOGIN_PAUSE_SCREEN)
        fn = pauseScreen;
    else if (pState > LOGIN_START && pState < LOGIN_END)
        fn = login;
    else if (pState > CREATE_START && pState < CREATE_END)
        fn = createPlayer;
    else if (pState > CON_START && pState < CON_END)
        fn = command;
    else if (pState > CON_STATS_START && pState < CON_STATS_END)
        fn = changingStats;
    else if (pState == CON_CONFIRM_SURNAME)
        fn = doSurname;
    else if (pState == CON_CONFIRM_TITLE)
        fn = doTitle;
    else if (pState == CON_VIEWING_FILE)
        fn = viewFile;
    else if (pState == CON_SENDING_MAIL)
        fn = postedit;
    else if (pState == CON_EDIT_NOTE)
        fn = noteedit;
    else if (pState == CON_EDIT_HISTORY)
        fn = histedit;
    else if (pState == CON_EDIT_PROPERTY)
        fn = Property::descEdit;
    else if (pState == CON_VIEWING_FILE_REVERSE)
        fn = viewFileReverse;
    else if (pState > CON_PASSWORD_START && pState < CON_PASSWORD_END)
        fn = changePassword;
    else if (pState == CON_CHOSING_WEAPONS)
        fn = convertNewWeaponSkills;
    else {
        std::clog << "Unknown connected state!\n";
        fn = login;
    }

    fnparam = (char) pFnParam;
}

bstring getMxpTag( bstring tag, bstring text ) {
    bstring::size_type n = text.find(tag);
    if(n == bstring::npos)
        return("");

    std::ostringstream oStr;
    // Add the legnth of the tag
    n += tag.length();
    if( n < text.length()) {
        // If our first char is a quote, advance
        if(text[n] == '\"')
            n++;
        while(n < text.length()) {
            char ch = text[n++];
            if(ch == '.' || isdigit(ch) || isalpha(ch) ) {
                oStr << ch;
           } else {
               return(oStr.str());
           }
        }
    }
    return("");
}
char caster_from_unsigned( unsigned char ch )
{
  return static_cast< char >( ch );
}

bool Socket::parseMXPSecure() {
    if(getMxp()) {
        bstring toParse(reinterpret_cast<char*>(&cmdInBuf[0]), cmdInBuf.size());
        std::clog << toParse << std::endl;

        bstring client = getMxpTag("CLIENT=", toParse);
        if (!client.empty()) {
            // Overwrite the previous client name - this is harder to fake
            term.type = client;
        }

        bstring version = getMxpTag("VERSION=", toParse);
        if(!version.empty()) {
            term.version = version;
            if(term.type.equals("mushclient", false)) {
                if(version >= "4.02")
                    opts.xterm256 = true;
                else
                    opts.xterm256 = false;
            } else if (term.type.equals("cmud", false)) {
                if(version >=  "3.04")
                    opts.xterm256 = true;
                else
                    opts.xterm256 = false;
            } else if (term.type.equals("atlantis", false)) {
                // Any version of atlantis with MXP supports xterm256
                opts.xterm256 = true;
            }
        }

        bstring supports = getMxpTag("SUPPORT=", toParse);
        if(!supports.empty()) {
            std::clog << "Got <SUPPORT='" << supports << "'>" << std::endl;
        }

    }
    clearMxpClientSecure();
    cmdInBuf.clear();
    return(true);
}

bool Socket::parseMsdp() {
    if(getMsdp()) {
        bstring var, val;
        int nest = 0;

        var.reserve(15);
        val.reserve(30);
        ssize_t i = 0, n = cmdInBuf.size();
        while (i < n && cmdInBuf[i] != SE) {
            switch (cmdInBuf[i]) {
            case MSDP_VAR:
                i++;
                while (i < n && cmdInBuf[i] != MSDP_VAL) {
                    var += cmdInBuf[i++];
                }
                break;
            case MSDP_VAL:
                i++;
                val.erase();
                while (i < n && cmdInBuf[i] != IAC) {
                    if (cmdInBuf[i] == MSDP_TABLE_OPEN
                            || cmdInBuf[i] == MSDP_ARRAY_OPEN)
                        nest++;
                    else if (cmdInBuf[i] == MSDP_TABLE_CLOSE
                            || cmdInBuf[i] == MSDP_ARRAY_CLOSE)
                        nest--;
                    else if (nest == 0
                            && (cmdInBuf[i] == MSDP_VAR || cmdInBuf[i] == MSDP_VAL))
                        break;
                    val += cmdInBuf[i++];
                }
                if (nest == 0)
                    processMsdpVarVal(var, val);

                break;
            default:
                i++;
                break;
            }
        }
    }
    cmdInBuf.clear();

    return (true);
}

bool Socket::parseAtcp() {
    if(getAtcp()) {
        bstring var, val;

        var.reserve(15);
        val.reserve(30);
        ssize_t i = 0, n = cmdInBuf.size();
        while (i < n && cmdInBuf[i] != SE) {
            switch (cmdInBuf[i]) {
            case '@':
                i++;
                var.erase();
                while (i < n && cmdInBuf[i] != ' ') {
                    var += cmdInBuf[i++];
                }
                break;
            case ' ':
                i++;
                val.erase();
                while (i < n && cmdInBuf[i] != '@' && cmdInBuf[i] != ' ') {
                    val += cmdInBuf[i++];
                }
                processMsdpVarVal(var, val);

                break;
            default:
                i++;
                break;
            }
        }
    }
    cmdInBuf.clear();

    return (true);
}


//********************************************************************
//                      bprint
//********************************************************************
// Append a string to the socket's output queue

void Socket::bprint(bstring toPrint) {
    if (!toPrint.empty())
        output += toPrint;
}

//********************************************************************
//                      bprintColor
//********************************************************************
// Append a string to the socket's output queue...in color!

void Socket::bprintColor(bstring toPrint) {
    if(!toPrint.empty()) {
        // Append the string to the output buffer, we'll parse color there
        output += toPrint;
    }
}

//********************************************************************
//                      bprintColor
//********************************************************************
// Append a string to the socket's output queue...without color!

void Socket::bprintNoColor(bstring toPrint) {
    if(!toPrint.empty()) {
        // Double any color codes to prevent them from being parsed later
        toPrint.Replace("^", "^^");
        output += toPrint;
    }

}
//********************************************************************
//                      println
//********************************************************************
// Append a string to the socket's output queue with a \n

void Socket::println(bstring toPrint) {
    bprint(toPrint + "\n");
    return;
}

//********************************************************************
//                      print
//********************************************************************

void Socket::print(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    bstring newFmt = stripColor(fmt);
    vprint( newFmt.c_str(), ap);
    va_end(ap);
}

//********************************************************************
//                      printColor
//********************************************************************

void Socket::printColor(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vprint(fmt, ap);
    va_end(ap);
}

//********************************************************************
//                      flush
//********************************************************************
// Flush pending output and send a prompt

void Socket::flush() {
    ssize_t n, len;
    if(!processed_output.empty()) {
        len = processed_output.length();

        n = write(processed_output, false, false);
    } else {
        len = output.length();
        if ((n = write(output)) == 0)
            return;
        output.clear();
    }
    // If we only wrote OOB data or partial data was written because of EWOULDBLOCK,
    // then n is -2, don't send a prompt in that case
    if (n != -2 && myPlayer && connState != CON_CHOSING_WEAPONS)
        myPlayer->sendPrompt();
}

//********************************************************************
//                      write
//********************************************************************
// Write a string of data to the the socket's file descriptor

ssize_t Socket::write(bstring toWrite, bool pSpy, bool process) {
    ssize_t written = 0;
    ssize_t n = 0;
    size_t total = 0;

    // Parse any color, unicode, etc here
    bstring toOutput;
    if(process)
        toOutput = parseForOutput(toWrite);
    else {
        toOutput = toWrite;
    }

    total = toOutput.length();

    const char *str = toOutput.c_str();
    // Write directly to the socket, otherwise compress it and send it
    if (!opts.compressing) {
        do {
            n = ::write(fd, str + written, total - written);
            if (n < 0) {
                if(errno != EWOULDBLOCK)
                    return (n);
                else  {
                    // The write would have blocked
                    n = -2;
                    // If we haven't written the total number of bytes planned
                    // Save the remaining string for the next go around
                    if(written < total) {
                        processed_output = str+written;
                    }
                    break;
                }
            }
            written += n;
        } while (written < total);

        UnCompressedBytes += written;

        if(n == -2)
            written = -2;

        if(written >= total && !processed_output.empty() && !process) {
            processed_output.erase();
        }
    } else {
        UnCompressedBytes += total;

        out_compress->next_in = (unsigned char*) str;
        out_compress->avail_in = total;
        while (out_compress->avail_in) {
            out_compress->avail_out =
                    COMPRESSED_OUTBUF_SIZE
                            - ((char*) out_compress->next_out
                                    - (char*) out_compress_buf);
            if (deflate(out_compress, Z_SYNC_FLUSH) != Z_OK) {
                return (0);
            }
            written += processCompressed();
            if (written == 0)
                break;
        }
    }

    if (pSpy && !spying.empty()) {
        bstring forSpy = Socket::stripTelnet(toWrite);

        forSpy.Replace("\n", "\n<Spy> ");
        if(!forSpy.empty()) {
            std::list<Socket*>::iterator it;
            for (it = spying.begin(); it != spying.end(); it++) {
                Socket *sock = *it;
                if (sock)
                    sock->write("<Spy> " + forSpy, false);
            }
        }
    }
    // Keep track of total outbytes
    OutBytes += written;

    // If stripped len is 0, it means we only wrote OOB data, so adjust the return so
    // we don't send another prompt
    if(!needsPrompt(toWrite))
        written = -2;

    return (written);
}

//--------------------------------------------------------------------
// MCCP

//********************************************************************
//                      startCompress
//********************************************************************

int Socket::startCompress(bool silent) {
    if (!opts.mccp)
        return (-1);

    if (opts.compressing)
        return (-1);

    out_compress_buf = new char[COMPRESSED_OUTBUF_SIZE];
    //out_compress = new z_stream;
    out_compress = (z_stream *) malloc(sizeof(*out_compress));
    out_compress->zalloc = telnet::zlib_alloc;
    out_compress->zfree = telnet::zlib_free;
    out_compress->opaque = nullptr;
    out_compress->next_in = nullptr;
    out_compress->avail_in = 0;
    out_compress->next_out = (Bytef*) out_compress_buf;
    out_compress->avail_out = COMPRESSED_OUTBUF_SIZE;

    if (deflateInit(out_compress, 9) != Z_OK) {
        // Problem with zlib, try to clean up
        delete out_compress_buf;
        free(out_compress);
        return (-1);
    }

    if (!silent) {
        if (opts.mccp == 2)
            write(telnet::start_mccp2, false);
        else
            write(telnet::start_mccp, false);
    }
    // We're compressing now
    opts.compressing = true;

    return (0);
}

//********************************************************************
//                      endCompress
//********************************************************************

int Socket::endCompress() {
    if (out_compress && opts.compressing) {
        unsigned char dummy[1] = { 0 };
        out_compress->avail_in = 0;
        out_compress->next_in = dummy;
        // process any remaining output first?
        if (deflate(out_compress, Z_FINISH) != Z_STREAM_END) {
            std::clog << "Error with deflate Z_FINISH\n";
            return (-1);
        }

        // Send any residual data
        if (processCompressed() < 0)
            return (-1);

        deflateEnd(out_compress);

        delete[] out_compress_buf;

        out_compress = nullptr;
        out_compress_buf = nullptr;

        opts.mccp = 0;
        opts.compressing = false;
    }
    return (-1);
}

//********************************************************************
//                      processCompressed
//********************************************************************

int Socket::processCompressed() {
    size_t len = (size_t) ((char*) out_compress->next_out - (char*) out_compress_buf);
    int written = 0;
    size_t block;
    ssize_t n, i;

    if (len > 0) {
        for (i = 0, n = 0; i < len; i += n) {
            block = MIN<size_t>(len - i, 4096);
            if ((n = ::write(fd, out_compress_buf + i, block)) < 0)
                return (-1);
            written += n;
            if (n == 0)
                break;
        }
        if (i) {
            if (i < len)
                memmove(out_compress_buf, out_compress_buf + i, len - i);

            out_compress->next_out = (Bytef*) out_compress_buf + len - i;
        }
    }
    return (written);
}
// End - MCCP
//--------------------------------------------------------------------

// "Telopts"
bool Socket::saveTelopts(xmlNodePtr rootNode) {
    rootNode = xml::newStringChild(rootNode, "Telopts");
    xml::newNumChild(rootNode, "MCCP", getMccp());
    xml::newNumChild(rootNode, "MSDP", getMsdp());
    xml::newNumChild(rootNode, "ATCP", getAtcp());
    xml::newBoolChild(rootNode, "MXP", getMxp());
    xml::newBoolChild(rootNode, "DumbClient", isDumbClient());
    xml::newStringChild(rootNode, "Term", getTermType());
    xml::newNumChild(rootNode, "Color", getColorOpt());
    xml::newNumChild(rootNode, "TermCols", getTermCols());
    xml::newNumChild(rootNode, "TermRows", getTermRows());
    xml::newBoolChild(rootNode, "EOR", getEor());
    xml::newBoolChild(rootNode, "Charset", getCharset());
    xml::newBoolChild(rootNode, "UTF8", getUtf8());

    return (true);
}
bool Socket::loadTelopts(xmlNodePtr rootNode) {
    xmlNodePtr curNode = rootNode->children;

    while (curNode) {
        if (NODE_NAME(curNode, "MCCP")) {
            int mccp = 0;
            xml::copyToNum(mccp, curNode);
            if (mccp) {
                write(telnet::will_comp2, false);
            }
        }
        else if (NODE_NAME(curNode, "MXP")) xml::copyToBool(opts.mxp, curNode);
        else if (NODE_NAME(curNode, "Color")) xml::copyToNum(opts.color, curNode);
        else if (NODE_NAME(curNode, "MSDP"))  xml::copyToBool(opts.msdp, curNode);
        else if (NODE_NAME(curNode, "ATCP")) {
            xml::copyToBool(opts.atcp, curNode);
            if (opts.atcp) {
                if (opts.msdp) {
                    opts.atcp = false;
                }
            }
        }
        else if (NODE_NAME(curNode, "Term")) xml::copyToBString(term.type, curNode);
        else if (NODE_NAME(curNode, "DumbClient")) xml::copyToBool(opts.dumb, curNode);
        else if (NODE_NAME(curNode, "TermCols")) xml::copyToNum(term.cols, curNode);
        else if (NODE_NAME(curNode, "TermRows")) xml::copyToNum(term.rows, curNode);
        else if (NODE_NAME(curNode, "EOR")) xml::copyToBool(opts.eor, curNode);
        else if (NODE_NAME(curNode, "Charset")) xml::copyToBool(opts.charset, curNode);
        else if (NODE_NAME(curNode, "UTF8")) xml::copyToBool(opts.UTF8, curNode);

        curNode = curNode->next;
    }

    // We can only have one of these enabled, MSDP trumps
    if (opts.msdp) {
        // Re-negotiate MSDP after a reboot
        write(telnet::will_msdp, false);
    } else if (opts.atcp) {
        // Re-negotiate ATCP after a reboot
        write(telnet::do_atcp, false);

    }

    return (true);
}

//********************************************************************
//                      hasOutput
//********************************************************************

bool Socket::hasOutput(void) const {
    return (!processed_output.empty() || !output.empty());
}

//********************************************************************
//                      hasCommand
//********************************************************************

bool Socket::hasCommand(void) const {
    return (!input.empty());
}

//********************************************************************
//                      canForce
//********************************************************************
// True if the socket is playing (ie: fn is command and fnparam is 1)

bool Socket::canForce(void) const {
    return (fn == (void(*)(Socket*, const bstring&)) ::command && fnparam == 1);
}

//********************************************************************
//                      get
//********************************************************************

int Socket::getState(void) const {
    return (connState);
}
bool Socket::isConnected() const {
    return (connState < LOGIN_START && connState != CON_DISCONNECTING);
}
bool Player::isConnected() const {
    return (getSock()->isConnected());
}

int Socket::getFd(void) const {
    return (fd);
}
bool Socket::getMxp(void) const {
    return (opts.mxp);
}
bool Socket::getMxpClientSecure() const {
    return(opts.mxpClientSecure);
}
void Socket::clearMxpClientSecure() {
    opts.mxpClientSecure = false;
}
int Socket::getMccp(void) const {
    return (opts.mccp);
}
bool Socket::getMsdp() const {
    return (opts.msdp);
}
;
bool Socket::getAtcp() const {
    return (opts.atcp);
}
;
bool Socket::getMsp() const {
    return (opts.msp);
}
;
bool Socket::getCharset() const {
    return (opts.charset);
}
;
bool Socket::getUtf8() const {
    return (opts.UTF8);
}
bool Socket::getEor(void) const {
    return (opts.eor);
}
bool Socket::isDumbClient(void) const {
    return(opts.dumb);
}
bool Socket::getNaws(void) const {
    return (opts.naws);
}
long Socket::getIdle(void) const {
    return (time(0) - ltime);
}
const bstring& Socket::getIp(void) const {
    return (host.ip);
}
const bstring& Socket::getHostname(void) const {
    return (host.hostName);
}
bstring Socket::getTermType() const {
    return (term.type);
}
int Socket::getColorOpt() const {
    return(opts.color);
}
void Socket::setColorOpt(int opt) {
    opts.color = opt;
}
int Socket::getTermCols() const {
    return (term.cols);
}
int Socket::getTermRows() const {
    return (term.rows);
}

int Socket::getParam() {
    return (fnparam);
}
void Socket::setParam(int newParam) {
    fnparam = newParam;
}

void Socket::setHostname(bstring pName) {
    host.hostName = pName;
}
void Socket::setIp(bstring pIp) {
    host.ip = pIp;
}
void Socket::setPlayer(Player* ply) {
    myPlayer = ply;
}

Player* Socket::getPlayer() const {
    return (myPlayer);
}

// MCCP Hooks
void *zlib_alloc(void *opaque, unsigned int items, unsigned int size) {
    return calloc(items, size);
}
void zlib_free(void *opaque, void *address) {
    free(address);
}

//********************************************************************
//                      nonBlock
//********************************************************************

int nonBlock(int pFd) {
    int flags;
    flags = fcntl(pFd, F_GETFL, 0);
    flags |= O_NONBLOCK;
    if (fcntl(pFd, F_SETFL, flags) < 0)
        return (-1);
    return (0);
}

//*********************************************************************
//                      showLoginScreen
//*********************************************************************

void Socket::showLoginScreen(bool dnsDone) {
    //*********************************************************************
    // As a part of the copyright agreement this section must be left intact
    //*********************************************************************
    print(
            "The Realms of Hell (RoH beta v" VERSION ")\n\tBased on Mordor by Brett Vickers, Brooke Paul.\n");
    print("Programmed by: Jason Mitchell, Randi Mitchell and Tim Callahan.\n");
    print("Contributions by: Jonathan Hseu.");

    char file[80];
    sprintf(file, "%s/login_screen.txt", Path::Config);
    viewLoginFile(this, file);

    if (dnsDone)
        checkLockOut();
    flush();
}

//********************************************************************
//                      askFor
//********************************************************************

void Socket::askFor(const char *str) {
    ASSERTLOG( str);
    if (getEor() == 1) {
        char eor_str[] = { (char) IAC, (char) EOR, '\0' };
        printColor(str);
        print(eor_str);
    } else {
        char ga_str[] = { (char) IAC, (char) GA, '\0' };
        printColor(str);
        print(ga_str);
    }
}

unsigned const char mssp_val[] = { MSSP_VAL, '\0' };
unsigned const char mssp_var[] = { MSSP_VAR, '\0' };

void addMSSPVar(std::ostringstream& msspStr, bstring var) {
    msspStr << mssp_var << var;
}

template<class T>
void addMSSPVal(std::ostringstream& msspStr, T val) {
    msspStr << mssp_val << val;
}

int Socket::sendMSSP() {
    std::clog << "Sending MSSP string\n";

    std::ostringstream msspStr;

    msspStr << telnet::sb_mssp_start;
    addMSSPVar(msspStr, "NAME");
    addMSSPVal<bstring>(msspStr, "The Realms of Hell");

    addMSSPVar(msspStr, "PLAYERS");
    addMSSPVal<int>(msspStr, gServer->getNumPlayers());

    addMSSPVar(msspStr, "UPTIME");
    addMSSPVal<long>(msspStr, StartTime);

    addMSSPVar(msspStr, "HOSTNAME");
    addMSSPVal<bstring>(msspStr, "mud.rohonline.net");

    addMSSPVar(msspStr, "PORT");
    addMSSPVal<bstring>(msspStr, "23");
    addMSSPVal<bstring>(msspStr, "3333");

    addMSSPVar(msspStr, "CODEBASE");
    addMSSPVal<bstring>(msspStr, "RoH beta v" VERSION);

    addMSSPVar(msspStr, "VERSION");
    addMSSPVal<bstring>(msspStr, "RoH beta v" VERSION);

    addMSSPVar(msspStr, "CREATED");
    addMSSPVal<bstring>(msspStr, "1998");

    addMSSPVar(msspStr, "LANGUAGE");
    addMSSPVal<bstring>(msspStr, "English");

    addMSSPVar(msspStr, "LOCATION");
    addMSSPVal<bstring>(msspStr, "United States");

    addMSSPVar(msspStr, "WEBSITE");
    addMSSPVal<bstring>(msspStr, "http://www.rohonline.net");

    addMSSPVar(msspStr, "FAMILY");
    addMSSPVal<bstring>(msspStr, "Mordor");

    addMSSPVar(msspStr, "GENRE");
    addMSSPVal<bstring>(msspStr, "Fantasy");

    addMSSPVar(msspStr, "GAMEPLAY");
    addMSSPVal<bstring>(msspStr, "Roleplaying");
    addMSSPVal<bstring>(msspStr, "Hack and Slash");
    addMSSPVal<bstring>(msspStr, "Adventure");

    addMSSPVar(msspStr, "STATUS");
    addMSSPVal<bstring>(msspStr, "Live");

    addMSSPVar(msspStr, "GAMESYSTEM");
    addMSSPVal<bstring>(msspStr, "Custom");

    addMSSPVar(msspStr, "AREAS");
    addMSSPVal<int>(msspStr, -1);

    addMSSPVar(msspStr, "HELPFILES");
    addMSSPVal<int>(msspStr, 1000);

    addMSSPVar(msspStr, "MOBILES");
    addMSSPVal<int>(msspStr, 5100);

    addMSSPVar(msspStr, "OBJECTS");
    addMSSPVal<int>(msspStr, 7500);

    addMSSPVar(msspStr, "ROOMS");
    addMSSPVal<int>(msspStr, 15000);

    addMSSPVar(msspStr, "CLASSES");
    addMSSPVal<int>(msspStr, gConfig->classes.size());

    addMSSPVar(msspStr, "LEVELS");
    addMSSPVal<int>(msspStr, MAXALVL);

    addMSSPVar(msspStr, "RACES");
    addMSSPVal<int>(msspStr, gConfig->getPlayableRaceCount());

    addMSSPVar(msspStr, "SKILLS");
    addMSSPVal<int>(msspStr, gConfig->skills.size());

    addMSSPVar(msspStr, "GMCP");
    addMSSPVal<bstring>(msspStr, "0");

    addMSSPVar(msspStr, "ATCP");
    addMSSPVal<bstring>(msspStr, "1");

    addMSSPVar(msspStr, "SSL");
    addMSSPVal<bstring>(msspStr, "0");

    addMSSPVar(msspStr, "ZMP");
    addMSSPVal<bstring>(msspStr, "0");

    addMSSPVar(msspStr, "PUEBLO");
    addMSSPVal<bstring>(msspStr, "0");

    addMSSPVar(msspStr, "MSDP");
    addMSSPVal<bstring>(msspStr, "1");

    addMSSPVar(msspStr, "MSP");
    addMSSPVal<bstring>(msspStr, "1");

    // TODO: UTF-8: Change to 1
    addMSSPVar(msspStr, "UTF-8");
    addMSSPVal<bstring>(msspStr, "0");

    addMSSPVar(msspStr, "VT100");
    addMSSPVal<bstring>(msspStr, "0");

    // TODO: XTERM 256: Change to 1
    addMSSPVar(msspStr, "XTERM 256 COLORS");
    addMSSPVal<bstring>(msspStr, "0");

    addMSSPVar(msspStr, "ANSI");
    addMSSPVal<bstring>(msspStr, "1");

    addMSSPVar(msspStr, "MCCP");
    addMSSPVal<bstring>(msspStr, "1");

    addMSSPVar(msspStr, "MXP");
    addMSSPVal<bstring>(msspStr, "1");

    addMSSPVar(msspStr, "PAY TO PLAY");
    addMSSPVal<bstring>(msspStr, "0");

    addMSSPVar(msspStr, "PAY FOR PERKS");
    addMSSPVal<bstring>(msspStr, "0");

    addMSSPVar(msspStr, "HIRING BUILDERS");
    addMSSPVal<bstring>(msspStr, "1");

    addMSSPVar(msspStr, "HIRING CODERS");
    addMSSPVal<bstring>(msspStr, "1");

    addMSSPVar(msspStr, "MULTICLASSING");
    addMSSPVal<bstring>(msspStr, "1");

    addMSSPVar(msspStr, "NEWBIE FRIENDLY");
    addMSSPVal<bstring>(msspStr, "1");

    addMSSPVar(msspStr, "PLAYER CLANS");
    addMSSPVal<bstring>(msspStr, "0");

    addMSSPVar(msspStr, "PLAYER CRAFTING");
    addMSSPVal<bstring>(msspStr, "1");

    addMSSPVar(msspStr, "PLAYER GUILDS");
    addMSSPVal<bstring>(msspStr, "1");

    addMSSPVar(msspStr, "EQUIPMENT SYSTEM");
    addMSSPVal<bstring>(msspStr, "Both");

    addMSSPVar(msspStr, "MULTIPLAYING");
    addMSSPVal<bstring>(msspStr, "Restricted");

    addMSSPVar(msspStr, "PLAYERKILLING");
    addMSSPVal<bstring>(msspStr, "Restricted");

    addMSSPVar(msspStr, "QUEST SYSTEM");
    addMSSPVal<bstring>(msspStr, "Integrated");

    addMSSPVar(msspStr, "ROLEPLAYING");
    addMSSPVal<bstring>(msspStr, "Encouraged");

    addMSSPVar(msspStr, "TRAINING SYSTEM");
    addMSSPVal<bstring>(msspStr, "Both");

    addMSSPVar(msspStr, "WORLD ORIGINALITY");
    addMSSPVal<bstring>(msspStr, "All Original");

    msspStr << telnet::sb_mssp_end;

    return (write(msspStr.str()));
}

