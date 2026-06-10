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
 *  Copyright (C) 2007-2021 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */


#include <arpa/telnet.h>                            // for IAC, SE, WILL, SB
#include <fcntl.h>                                  // for fcntl, F_GETFL
#include <fmt/format.h>                             // for format
#include <netinet/in.h>                             // for htonl, sockaddr_in
#include <sys/socket.h>                             // for linger, setsockopt
#include <unistd.h>                                 // for ssize_t, write
#include <zconf.h>                                  // for Bytef
#include <zlib.h>                                   // for z_stream, deflate
#include <algorithm>                                // for replace
#include <array>                                    // for array
#include <span>                                     // for span
#include <boost/algorithm/string/predicate.hpp>     // for iequals, istarts_...
#include <boost/algorithm/string/replace.hpp>       // for replace_all
#include <boost/token_functions.hpp>                // for char_separator
#include <boost/tokenizer.hpp>                      // for tokenizer
#include <cctype>                                   // for isalpha, isdigit
#include <charconv>                                 // for from_chars
#include <cerrno>                                   // for EWOULDBLOCK, errno
#include <cstdarg>                                  // for va_end, va_list
#include <cstdio>                                   // for fseek, size_t, ftell
#include <cstdlib>                                  // for free, atol, calloc
#include <cstring>                                  // for strlen, strcpy
#include <ctime>                                    // for time
#include <deque>                                    // for _Deque_iterator
#include <iostream>                                 // for operator<<, basic...
#include <fstream>                                  // for std::ifstream
#include <list>                                     // for list, operator==
#include <map>                                      // for map
#include <memory>                                   // for allocator, alloca...
#include <queue>                                    // for queue
#include <sstream>                                  // for basic_ostringstre...
#include <string>                                   // for string, basic_string
#include <string_view>                              // for string_view, basi...
#include <utility>
#include <vector>                                   // for vector

#include <asio/buffer.hpp>                          // for asio::buffer
#include <asio/read.hpp>                            // for async_read_some (via tcp::socket)
#include <asio/write.hpp>                           // for asio::async_write / asio::write

#include "color.hpp"                                // for stripColor
#include "commands.hpp"                             // for command, changing...
#include "config.hpp"                               // for Config, gConfig
#include "flags.hpp"                                // for P_READING_FILE
#include "global.hpp"                               // for MAXALVL
#include "login.hpp"                                // for createPlayer, CON...
#include "msdp.hpp"                                 // for ReportedMsdpVariable
#include "gmcp.hpp"                                 // for the pure GMCP conversion layer
#include "gmcpEvents.hpp"                            // for gmcp::commChannelList
#include "mud.hpp"                                  // for StartTime
#include "mudObjects/players.hpp"                   // for Player
#include "paths.hpp"                                // for Config
#include "post.hpp"                                 // for histedit, postedit
#include "property.hpp"                             // for Property
#include "proto.hpp"                                // for zero
#include "security.hpp"                             // for changePassword
#include "server.hpp"                               // for Server, gServer
#include "socket.hpp"                               // for Socket, Socket::S...
#include "version.hpp"                              // for VERSION
#include "xml.hpp"                                  // for copyToBool, newBo...
#include "blackjack.hpp"                            // for interactive gambling
#include "account.hpp"                              // for Account

constexpr int MIN_PAGES = 10;

// Static initialization
int Socket::numSockets = 0;

using enum Socket::TelnetState;

//********************************************************************
//                      telnet namespace
//********************************************************************

namespace telnet {

long parseMtts(std::string_view ttype) {
    constexpr std::string_view prefix = "MTTS ";
    if (ttype.substr(0, prefix.size()) != prefix) return 0;
    auto rest = ttype.substr(prefix.size());
    long bits = 0;
    auto [ptr, ec] = std::from_chars(rest.data(), rest.data() + rest.size(), bits);
    if (ec != std::errc{} || ptr == rest.data()) return 0;
    return bits;
}

std::string mttsCaps(long bits) {
    std::string out;
    if (bits & MTTS_ANSI) out += "ANSI ";
    if (bits & MTTS_VT100) out += "VT100 ";
    if (bits & MTTS_UTF8) out += "UTF8 ";
    if (bits & MTTS_256COLOR) out += "256COLOR ";
    if (bits & MTTS_MOUSE) out += "MOUSE ";
    if (bits & MTTS_COLORPALETTE) out += "COLORPALETTE ";
    if (bits & MTTS_SCREENREADER) out += "SCREENREADER ";
    if (bits & MTTS_PROXY) out += "PROXY ";
    if (bits & MTTS_TRUECOLOR) out += "TRUECOLOR ";
    if (bits & MTTS_MNES) out += "MNES ";
    if (bits & MTTS_MSLP) out += "MSLP ";
    if (bits & MTTS_SSL) out += "SSL ";
    if (!out.empty()) out.pop_back();   // drop trailing space
    return out;
}

// MCCP Hooks
void *zlib_alloc(void *opaque, unsigned int items, unsigned int size) {
    return calloc(items, size);
}
void zlib_free(void *opaque, void *address) {
    free(address);
}

std::string promptGoAhead(bool eor, bool dumb) {
    if(eor) return std::string(reinterpret_cast<const char*>(eor_str));
    if(!dumb) return std::string(reinterpret_cast<const char*>(ga_str));
    return std::string();
}

std::string escapeIAC(std::string_view in) {
    if(in.find(static_cast<char>(IAC)) == std::string_view::npos)
        return std::string(in);
    std::string out;
    out.reserve(in.size() + 8);
    for(const char c : in) {
        out += c;
        if(static_cast<unsigned char>(c) == IAC)
            out += static_cast<char>(IAC);
    }
    return out;
}

std::string unescapeIAC(std::string_view in) {
    std::string out;
    out.reserve(in.size());
    for(size_t i = 0; i < in.size(); ++i) {
        const unsigned char c = static_cast<unsigned char>(in[i]);
        out += static_cast<char>(c);
        if(c == IAC && i + 1 < in.size() && static_cast<unsigned char>(in[i + 1]) == IAC)
            ++i;
    }
    return out;
}

std::string subnegotiate(unsigned char telopt, std::string_view payload, bool escapePayload) {
    const std::string esc = escapePayload ? escapeIAC(payload) : std::string(payload);
    std::string out;
    out.reserve(esc.size() + 5);
    out.push_back(static_cast<char>(IAC));
    out.push_back(static_cast<char>(SB));
    out.push_back(static_cast<char>(telopt));
    out += esc;
    out.push_back(static_cast<char>(IAC));
    out.push_back(static_cast<char>(SE));
    return out;
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
    opts.gmcp = false;
    opts.charset = false;
    opts.utf8 = false;
    opts.mxpClientSecure = false;
    opts.color = NO_COLOR;
    opts.xterm256 = false;
    opts.lastColor = '\0';

    opts.compressing = false;

    outCompress.reset();
    outCompressBuf.clear();
    myPlayer = nullptr;
    currentAccountName.clear();

    tState = NEG_NONE;
    oneIAC = watchBrokenClient = false;

    term.type = "dumb";
    term.firstType.clear();
    term.cols = 82;
    term.rows = 40;

    ltime = time(nullptr);
    intrpt = 0;

    fn = nullptr;
    tState = NEG_NONE;
    connState = LOGIN_START;
    lastState = LOGIN_START;

    zero(tempstr, sizeof(tempstr));
    inBuf.clear();
    spyingOn.reset();
}

//********************************************************************
//                      Socket
//********************************************************************

Socket::Socket(asio::ip::tcp::socket pSock) {
    reset();
    sock = std::make_unique<asio::ip::tcp::socket>(std::move(pSock));
    fd = static_cast<int>(sock->native_handle());

    // Rebuild a sockaddr_in from the peer endpoint for resolveIp + the DNS resolver fork.
    sockaddr_in addr{};
    asio::error_code ec;
    auto ep = sock->remote_endpoint(ec);
    const bool haveEndpoint = !ec;
    if(haveEndpoint) {
        addr.sin_family = AF_INET;
        addr.sin_port = htons(ep.port());
        addr.sin_addr.s_addr = htonl(ep.address().to_v4().to_uint());
    }

    resolveIp(addr, host.ip);
    resolveIp(addr, host.hostName); // start off with the hostname as the ip, then do an asynchronous lookup

    asio::error_code lec;
    sock->set_option(asio::socket_base::linger(false, 0), lec);

    numSockets++;

    // If we're running under valgrind, we don't resolve dns.  The child process tends to mess with proper memory leak detection.
    // With no peer endpoint (remote_endpoint failed) there's nothing to resolve -- proceed on the ip string.
    if (!haveEndpoint || gServer->getDnsCache(host.ip, host.hostName) || gServer->isValgrind()) {
        dnsDone = true;
    } else {
        dnsDone = false;
        setState(LOGIN_DNS_LOOKUP);
        gServer->startDnsLookup(this, addr);
    }
}

Socket::Socket(int pFd) {
    reset();
    fd = pFd;
    numSockets++;
}

//********************************************************************
//                      startRead / doWrite / enqueue (asio I/O)
//********************************************************************

// A client that can't keep up must not balloon memory: cap queued output bytes,
// and cap unprocessed input commands (pausing reads applies TCP backpressure).
static constexpr size_t kMaxQueuedBytes = 1u << 20;   // 1 MB of outbound backlog
static constexpr size_t kMaxQueuedCommands = 1024;    // pending input commands before we pause reads

void Socket::startRead() {
    if(!sock || !sock->is_open()) return;
    auto self = shared_from_this();
    sock->async_read_some(asio::buffer(readBuf),
        [this, self](const asio::error_code& ec, std::size_t n) {
            if(ec) {
                setState(CON_DISCONNECTING);
                return;
            }
            InBytes += static_cast<long>(n);
            std::string decoded;
            decoded.reserve(n);
            decodeBytes(std::span(readBuf.data(), n), decoded);
            extractCommands(std::move(decoded));
            ltime = time(nullptr);
            // Pause reads when the command backlog is high; resumeRead() re-arms once the
            // game loop drains it. Leaving the socket unarmed backpressures the sender via TCP.
            if(input.size() >= kMaxQueuedCommands)
                readPaused = true;
            else
                startRead();
        });
}

void Socket::resumeRead() {
    if(readPaused && input.size() < kMaxQueuedCommands) {
        readPaused = false;
        startRead();
    }
}

void Socket::enqueue(std::string bytes) {
    if(bytes.empty())
        return;
    if(queuedBytes + bytes.size() > kMaxQueuedBytes) {
        std::clog << "Socket " << fd << ": output backlog exceeded " << kMaxQueuedBytes << " bytes, disconnecting\n";
        if(writeInFlight)
            writeQueue.erase(std::next(writeQueue.begin()), writeQueue.end());
        else
            writeQueue.clear();
        queuedBytes = writeQueue.empty() ? 0 : writeQueue.front().size();
        setState(CON_DISCONNECTING);
        return;
    }
    queuedBytes += bytes.size();
    writeQueue.emplace_back(std::move(bytes));
    doWrite();
}

void Socket::doWrite() {
    if(writeQueue.empty())
        return;

    if(sock) {
        if(writeInFlight || !sock->is_open())
            return;
        auto self = weak_from_this().lock();
        if(!self)
            return;
        writeInFlight = true;
        asio::async_write(*sock, asio::buffer(writeQueue.front()),
            [this, self](const asio::error_code& ec, std::size_t /*n*/) {
                writeInFlight = false;
                if(ec) {
                    setState(CON_DISCONNECTING);
                    return;
                }
                if(writeQueue.empty()) return;
                queuedBytes -= writeQueue.front().size();
                writeQueue.pop_front();
                if(!writeQueue.empty())
                    doWrite();
            });
        return;
    }

    if(fd < 0)
        return;
    while(!writeQueue.empty()) {
        std::string& front = writeQueue.front();
        const ssize_t n = ::write(fd, front.data(), front.size());
        if(n < 0) {
            if(errno != EWOULDBLOCK)
                setState(CON_DISCONNECTING);
            break;
        }
        if(static_cast<size_t>(n) < front.size()) {
            queuedBytes -= static_cast<size_t>(n);
            front.erase(0, static_cast<size_t>(n));
            break;
        }
        queuedBytes -= front.size();
        writeQueue.pop_front();
    }
}

void Socket::drainAndClose() {
    if(sock) {
        asio::error_code ec;
        if(!writeInFlight) {
            asio::error_code nbec;
            sock->non_blocking(true, nbec);
            while(!writeQueue.empty()) {
                const std::size_t w = sock->write_some(asio::buffer(writeQueue.front()), ec);
                if(ec || w < writeQueue.front().size())
                    break;
                writeQueue.pop_front();
            }
        }
        sock->close(ec);
    } else if(fd >= 0) {
        while(!writeQueue.empty()) {
            const std::string& front = writeQueue.front();
            [[maybe_unused]] ssize_t n = ::write(fd, front.data(), front.size());
            writeQueue.pop_front();
        }
        close(fd);
    }
    writeQueue.clear();
    queuedBytes = 0;
    writeInFlight = false;
    fd = -1;
}

//********************************************************************
//                      closeFd
//********************************************************************
// Disconnect the underlying file descriptor
void Socket::cleanUp() {
    if(cleanedUp) return;
    cleanedUp = true;

    clearSpying();
    clearSpiedOn();
    msdpClearReporting();

    const std::string accountName = getAccountName();
    if (myPlayer) {
        // Save the player before untracking the account: untrackAccountConnection can evict the
        // account from gServer's cache, after which getAccount() returns null and the save is lost.
        if (myPlayer->fd > -1) {
            myPlayer->save(true);
            myPlayer->uninit();
        }
        const std::string characterName = myPlayer->getName();
        if(!accountName.empty() && !characterName.empty() && gServer) {
            gServer->untrackAccountConnection(accountName, characterName);
        }
        if(registered) {
            gServer->clearPlayer(myPlayer->getName());
            registered=false;
        }
        myPlayer = nullptr;
    } else if(!accountName.empty() && gServer) {
        // If we are at the account menu (no player), release the cached account
        gServer->releaseAccount(accountName, "");
    }
    currentAccountName.clear();
    endCompress();
    drainAndClose();
}
//********************************************************************
//                      ~Socket
//********************************************************************

Socket::~Socket() {
    numSockets--;
    if(!cleanedUp) {
        std::clog << "Socket destroyed without prior cleanUp (teardown invariant violated)\n";
        cleanedUp = true;
        endCompress();
        drainAndClose();
    }
}

// End - Constructors, Destructors, etc
//--------------------------------------------------------------------

//********************************************************************
//                      clearSpying
//********************************************************************

void Socket::clearSpying() {
    if (auto spied = spyingOn.lock()) {
        spied->removeSpy(this);
        spyingOn.reset();
    }
}

//********************************************************************
//                      clearSpiedOn
//********************************************************************

void Socket::clearSpiedOn() {
    for (auto & it : spying) {
        auto sock = it.lock();
        if (sock)
            sock->setSpying(nullptr);
    }
    spying.clear();
}

//********************************************************************
//                      setSpying
//********************************************************************

void Socket::setSpying(const std::shared_ptr<Socket>& sock) {
    if (sock)
        clearSpying();
    spyingOn = sock;

    if (sock)
        sock->addSpy(shared_from_this());
    if (!sock) {
        if (myPlayer)
            myPlayer->clearFlag(P_SPYING);
    }
}

//********************************************************************
//                      removeSpy
//********************************************************************

void Socket::removeSpy(Socket *sock) {
    spying.remove_if([&sock](const std::weak_ptr<Socket>& ptr) {
        return ptr.lock().get() == sock;
    });

    if (myPlayer && sock->myPlayer && myPlayer->getClass() >= sock->myPlayer->getClass())
        sock->printColor("^r%s is no longer observing you.\n", sock->myPlayer->getCName());
}

//********************************************************************
//                      addSpy
//********************************************************************

void Socket::addSpy(const std::shared_ptr<Socket>& sock) {
    spying.push_back(sock);
}

//********************************************************************
//                      disconnect
//********************************************************************
// Set this socket for removal on the next cleanup

void Socket::disconnect() {
    setState(CON_DISCONNECTING);
    flush();
    cleanUp();
}

//********************************************************************
//                      resolveIp
//********************************************************************

void Socket::resolveIp(const sockaddr_in &addr, std::string& ip) {
    std::ostringstream tmp;
    const long i = htonl(addr.sin_addr.s_addr);
    tmp << ((i >> 24) & 0xff) << "." << ((i >> 16) & 0xff) << "." << ((i >> 8) & 0xff) << "." << (i & 0xff);
    ip = tmp.str();
}

std::string Socket::parseForOutput(std::string_view outBuf) {
    int i = 0;
    auto n = outBuf.size();
    std::string oStr;
    oStr.reserve(n);
    bool inTag = false;
    unsigned char ch = 0;
    while(i < n) {
        ch = outBuf[i++];
        if(inTag) {
            if(ch == CH_MXP_END) {
                inTag = false;
                if(opts.mxp)
                    oStr += ">" MXP_LOCK_CLOSE;
            } else if(opts.mxp) {
                oStr += static_cast<char>(ch);
            }

            continue;
        } else {
            if(ch == CH_MXP_BEG) {
                inTag = true;
                if(opts.mxp)
                    oStr += MXP_SECURE_OPEN "<";
                continue;
            } else {
                if(ch == '^') {
                    if(i >= n) // dangling caret at end of buffer: nothing to escape
                        break;
                    ch = outBuf[i++];
                    oStr += getColorCode(ch);
                } else if(ch == '\n') {
                    oStr += "\r\n";
                } else if(ch == CH_GO_AHEAD) {
                    // Prompt go-ahead: emit the raw telnet marker in place (never doubled).
                    oStr += telnet::promptGoAhead(opts.eor, opts.dumb);
                } else if(ch == IAC) {
                    // RFC 854: a literal 0xFF in the data stream must be doubled.
                    oStr += static_cast<char>(IAC);
                    oStr += static_cast<char>(IAC);
                } else {
                    oStr += static_cast<char>(ch);
                }
                continue;
            }
        }
    }
    return oStr;
}

std::size_t Socket::skipTelnetSeq(std::string_view in, std::size_t i) {
    auto n = in.size();
    if(i + 1 >= n)  // dangling IAC: incomplete command
        return n;
    switch(static_cast<unsigned char>(in[i+1])) {
        case WILL:
        case WONT:
        case DO:
            return i + telnet::TELNET_OPT_CMD_LEN;
        case EOR:
            return i + telnet::TELNET_CMD_LEN;
        case SB:
            i += telnet::TELNET_CMD_LEN; // past IAC SB
            while(i + 1 < n) {
                if(static_cast<unsigned char>(in[i]) == IAC && static_cast<unsigned char>(in[i+1]) == SE)
                    return i + telnet::TELNET_CMD_LEN;   // past IAC SE
                i++;
            }
            return n; // unterminated SB: consume the rest
    }
    return i + 1; // IAC + unknown byte: skip the IAC only
}

bool Socket::needsPrompt(std::string_view inStr) {
    std::size_t i = 0;
    auto n = inStr.size();

    while(i < n) {
        if(static_cast<unsigned char>(inStr[i]) == IAC) {
            i = skipTelnetSeq(inStr, i);
            if(i >= n)   // only (possibly truncated) telnet: no real output
                return false;
            continue;
        }
        return true;
    }
    return false;
}

std::string Socket::stripTelnet(std::string_view inStr) {
    std::size_t i = 0;
    auto n = inStr.size();
    std::string oStr;
    oStr.reserve(n);

    while(i < n) {
        if(static_cast<unsigned char>(inStr[i]) == IAC) {
            i = skipTelnetSeq(inStr, i);    // drop the telnet command (truncated -> consumes rest)
            continue;
        }
        oStr += inStr[i++];
    }
    return oStr;
}

//********************************************************************
//                      checkLockOut
//********************************************************************

void Socket::checkLockOut() {
    const int lockStatus = gConfig->isLockedOut(shared_from_this());
    if (lockStatus == 0) {
        print("\n\nAn account can hold several characters.\nLegacy characters can be claimed by an account.\n\nLogin Options:");
        print("\n  ^Wa^x) Create or Login into an account");
        print("\n  ^Wb^x) Login in with a character name directly (Legacy)");
        askFor("\n\nEnter choice (a/b): ");
        setState(LOGIN_ENTRY_CHOICE);
    } else if (lockStatus == 2) {
        print("\n\nA password is required to play from your site: ");
        setState(LOGIN_GET_LOCKOUT_PASSWORD);
    } else if (lockStatus == 1) {
        print("\n\nYou are not welcome here. Begone.\n");
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

    writeRaw(telnet::do_ttype);

}
void Socket::continueTelnetNeg(bool queryTType) {
    if (queryTType)
        writeRaw(telnet::query_ttype);

    // Not a dumb client if we've gotten a response
    opts.dumb = false;
    writeRaw(telnet::will_comp2);

    writeRaw(telnet::do_naws);
    writeRaw(telnet::will_msdp);
    writeRaw(telnet::will_gmcp);
    writeRaw(telnet::will_mssp);
    writeRaw(telnet::will_msp);
    writeRaw(telnet::do_charset);
    writeRaw(telnet::do_new_environ);
    writeRaw(telnet::will_mxp);
    writeRaw(telnet::will_eor);
}

//********************************************************************
//                      processInput
//********************************************************************

int Socket::processInput() {
    std::array<unsigned char, 1024> buf;

    // Attempt to read from the socket
    const ssize_t n = read(getFd(), buf.data(), buf.size());
    if (n <= 0) return errno == EWOULDBLOCK ? 0 : -1;

    InBytes += n;

    std::string decoded;
    decoded.reserve(static_cast<std::size_t>(n));
    decodeBytes(std::span(buf.data(), static_cast<std::size_t>(n)), decoded);
    extractCommands(std::move(decoded));

    ltime = time(nullptr);
    return 0;
}

void Socket::decodeBytes(std::span<const unsigned char> data, std::string& tmp) {
    const unsigned char* tmpBuf = data.data();
    const ssize_t n = static_cast<ssize_t>(data.size());

    // Look for any IAC commands using a finite state machine
    for (ssize_t i = 0; i < n; i++) {
        // For debugging
//        std::clog << "DEBUG:" << (unsigned int)tmpBuf[i] << "'" << (unsigned char)tmpBuf[i] << "'" << "\n";

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
            if (tState == NEG_NONE && tmpBuf[i] == SE) {
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
                if (tmpBuf[i] == IAC) {
                    tState = NEG_IAC;
                    break;
                } else if(tmpBuf[i] == '\033') {
                    tState = NEG_MXP_SECURE;
                    break;
                } else if(tmpBuf[i] == CH_GO_AHEAD) {
                    // Internal-only output sentinel; never accept it from a client.
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
                    tmp += '\033';
                    tmp += static_cast<char>(tmpBuf[i]);
                }
                tState = NEG_NONE;
                break;
            case NEG_MXP_SECURE_TWO:
                if(tmpBuf[i] == '1') {
                    tState = NEG_MXP_SECURE_FINISH;
                    break;
                } else {
                    tmp += "\033[";
                    tmp += static_cast<char>(tmpBuf[i]);
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
                    tmp += "\033[1";
                    tmp += static_cast<char>(tmpBuf[i]);
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
                switch (tmpBuf[i]) {
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
                negotiate(tmpBuf[i]);
                break;
            case NEG_SB:
                switch (tmpBuf[i]) {
                    case NAWS:
                        tState = NEG_SB_NAWS_COL_HIGH;
                        break;
                    case TTYPE:
                        tState = NEG_SB_TTYPE;
                        break;
                    case CHARSET:
                        if (charsetEnabled())
                            tState = NEG_SB_CHARSET;
                        else
                            tState = NEG_NONE;
                        break;
                    case MSDP:
                        tState = NEG_SB_MSDP;
                        break;
                    case TELOPT_GMCP:
                        tState = NEG_SB_GMCP;
                        break;
                    case TELOPT_NEW_ENVIRON:
                        cmdInBuf.clear();
                        tState = NEG_SB_NEW_ENVIRON;
                        break;
                    default:
                        std::clog << "Unknown Sub Negotiation: " << static_cast<int>(tmpBuf[i]) << std::endl;
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
                    // We should have a full MDSP command now, let's parse it now
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
                cmdInBuf.push_back(tmpBuf[i]);
                if(tmpBuf[i] == IAC) {
                    tState = NEG_SB_GMCP_END;
                    break;
                }
                break;
            case NEG_SB_GMCP_END:
                if (tmpBuf[i] == SE) {
                    parseGmcp();
                    tState = NEG_NONE;
                    break;
                } else {
                    cmdInBuf.push_back(tmpBuf[i]);
                    tState = NEG_SB_GMCP;
                    break;
                }
                break;
            case NEG_SB_NEW_ENVIRON:
                if (tmpBuf[i] == IAC) {
                    tState = NEG_SB_NEW_ENVIRON_END;
                    break;
                }
                cmdInBuf.push_back(tmpBuf[i]);
                break;
            case NEG_SB_NEW_ENVIRON_END:
                if (tmpBuf[i] == SE) {
                    parseNewEnviron();
                    tState = NEG_NONE;
                    break;
                }
                // Doubled IAC inside the data
                cmdInBuf.push_back(IAC);
                cmdInBuf.push_back(tmpBuf[i]);
                tState = NEG_SB_NEW_ENVIRON;
                break;
            case NEG_SB_CHARSET:
                // We've only asked for UTF-8, so assume if they respond it's for that and just eat the rest of the input
                //
                // Any other sub-negotiations (such as TTABLE-*) are not handled
                if (tmpBuf[i] == ACCEPTED) {
                    std::clog << "Enabled UTF8" << std::endl;
                    opts.utf8 = true;
                    tState = NEG_SB_CHARSET_LOOK_FOR_IAC;
                } else if (tmpBuf[i] == REJECTED) {
                    // Don't downgrade UTF-8 the client already proved via MTTS bit 4.
                    opts.utf8 = (mtts & MTTS_UTF8) != 0;
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
                    std::clog << "NEG_SB_CHARSET_END Error: Expected SE, got '" << static_cast<int>(tmpBuf[i]) << "'" << std::endl;
                }
                tState = NEG_NONE;
                break;
            case NEG_SB_TTYPE:
                // Grab the terminal type
                if (tmpBuf[i] == TELQUAL_IS) {
                    term.lastType = term.type;
                    term.type.clear();
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
                    std::clog << "Found term type: " << term.type << std::endl;
                    // We haven't cycled back around to the first term type, and
                    // No previous term type or the current term type isn't the same as the last
                    if ((term.firstType != term.type) && (term.lastType.empty() ||  (term.type != term.lastType))) {
                        term.lastType = "";
                        // Look for 256 color support
                        if (term.type.find("-256color") != std::string::npos) {
                            // Works for tintin++, wintin++ and blowtorch
                            opts.xterm256 = true;
                        }

                        // Request another!
                        writeRaw(telnet::query_ttype);
                    }
                    if (term.firstType.empty()) {
                        term.firstType = term.type;
                    }

                    if (term.type.find("Mudlet") != std::string::npos and term.type > "Mudlet 1.1") {
                        opts.xterm256 = true;
                    } else if(boost::iequals(term.type, "EMACS-RINZAI") || term.type.find("DecafMUD") != std::string::npos) {
                        opts.xterm256 = true;
                    }

                    // MTTS: clients send "MTTS <bits>" as a later TTYPE IS in the cycle.
                    if (const long bits = telnet::parseMtts(term.type))
                        applyMtts(bits);

                } else if (tmpBuf[i] == IAC) {
                    // I doubt this will happen
                    std::clog << "NEG_SB_TTYPE: Found double IAC" << std::endl;
                    term.type += tmpBuf[i];
                    tState = NEG_SB_TTYPE;
                    break;
                } else {
                    std::clog << "NEG_SB_TTYPE_END Error: Expected SE, got '" << static_cast<int>(tmpBuf[i]) << "'" << std::endl;
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
}

void Socket::extractCommands(std::string decoded) {
    // Handles the screwy windows telnet, and its not that hard for
    // other clients that send \n\r too
    std::ranges::replace(decoded, '\r', '\n');

    const std::string::size_type start = inBuf.size();
    inBuf += decoded;

    // handle backspaces; a backspace at the start of the new input may erase
    // the tail of input buffered from an earlier read, hence the reach-back.
    std::string::size_type len = inBuf.size();
    for (std::string::size_type i = start; i < len; i++) {
        if (inBuf[i] == '\b' || inBuf[i] == 127) {
            if (i == 0) {
                inBuf.erase(i, 1);
                len -= 1;
                i--; // wraps; the loop's ++ restores it to 0
            } else {
                inBuf.erase(i - 1, 2);
                len -= 2;
                i--;
            }
        }
    }

    std::string::size_type idx = 0;
    while ((idx = inBuf.find('\n')) != std::string::npos) {
        std::string tmpr = inBuf.substr(0, idx); // Don't copy the \n
        idx += 1; // Consume the \n
        if (inBuf[idx] == '\n')
            idx += 1; // Consume the extra \n if applicable

        inBuf.erase(0, idx);
        if(opts.mxpClientSecure) {
            if(boost::istarts_with(inBuf, "<version")) {
                std::clog << "Got msxp version\n";
            } else if(boost::istarts_with(inBuf, "<supports")) {
                std::clog << "Got msxp supports\n";
            }
        }
        input.push(std::move(tmpr));
    }
}

bool Socket::negotiate(unsigned char ch) {

    switch (ch) {
        case TELOPT_CHARSET:
            if (tState == NEG_WILL) {
                opts.charset = true;
                writeRaw(telnet::charset_utf8);
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
                    writeRaw(telnet::wont_ttype);
                    continueTelnetNeg(false);

                }
            }
            tState = NEG_NONE;
            break;
        case TELOPT_MXP:
            if (tState == NEG_WILL || tState == NEG_DO) {
                writeRaw(telnet::start_mxp);
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
                opts.mccp = telnet::MCCP_V2;
                startCompress();
            } else if (tState == NEG_WONT || tState == NEG_DONT) {
                if (opts.mccp == telnet::MCCP_V2) {
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
            opts.naws = (tState == NEG_WILL);
            tState = NEG_NONE;
            break;
        case TELOPT_ECHO:
            // TODO: remote echo unimplemented
            tState = NEG_NONE;
            break;
        case TELOPT_NEW_ENVIRON:
            if (tState == NEG_WILL) {
                writeRaw(telnet::sb_new_environ_send);
                std::clog << "NEW-ENVIRON: requesting client vars" << std::endl;
            }
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

                if (gConfig->getLogTelnet()) std::clog << "Enabled MSDP" << std::endl;
            } else {
                if (gConfig->getLogTelnet()) std::clog << "Disabled MSDP" << std::endl;
                opts.msdp = false;
            }
            tState = NEG_NONE;
            break;
        case TELOPT_GMCP:
            if (tState == NEG_DO) {
                opts.gmcp = true;
                if (gConfig->getLogTelnet()) std::clog << "Enabled GMCP" << std::endl;
            } else {
                if (gConfig->getLogTelnet()) std::clog << "Disabled GMCP" << std::endl;
                opts.gmcp = false;
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

bool Socket::handleNaws(int& colRow, unsigned char chr, bool high) {
    // If we get an IAC here, we need a double IAC
    if (chr == IAC) {
        if (!oneIAC) {
            oneIAC = true;
            return (false);
        } else {
            oneIAC = false;
        }
    } else if (oneIAC && chr != IAC) {
        // Error!
        std::clog << "NAWS: BUG - Expecting a doubled IAC, got " << static_cast<unsigned int>(chr) << "\n";
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

int Socket::processOneCommand() {
    std::string cmd = input.front();
    input.pop();

    // Send the command to the people we're spying on
    if (!spying.empty()) {
        for (const auto& sIt : spying) {
            if (auto sock = sIt.lock()) sock->write(fmt::format("[{}]\n", cmd), false);
        }
    }

    fn(shared_from_this(), cmd);

    return (1);
}

//********************************************************************
//                      restoreState
//********************************************************************
// Returns a fd to its previous state

void Socket::restoreState() {
    setState(lastState);
    createPlayer(shared_from_this(), "");
}

//*********************************************************************
//                      pauseScreen
//*********************************************************************

void pauseScreen(const std::shared_ptr<Socket>& sock, const std::string &str) {
    if(str == "quit")
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

    if(myPlayer) {
        // TODO: Only clear if we're the one who registered the player
        myPlayer->uninit();   // remove from room/group/pets; clearPlayer alone leaves it dangling
        gServer->clearPlayer(myPlayer->getName());
        myPlayer = nullptr;
    }
    currentAccountName.clear();

    if (pauseScreen) {
        setState(LOGIN_PAUSE_SCREEN);
        printColor("\nPress ^W[RETURN]^x to reconnect or type ^Wquit^x to disconnect.\n: ");
    } else {
        showLoginScreen();
        askFor("\n\nPlease enter account name: ");
        setState(LOGIN_GET_ACCOUNT_NAME);
    }
}


void viewFileReverse(const std::shared_ptr<Socket>& sock, const std::string& file) {
    sock->viewFileReverse(file);
}


void handlePaging(const std::shared_ptr<Socket>& sock, const std::string& inStr) {
    sock->handlePaging(inStr);
}

void Socket::sendPages(int numPages) {
    for(int i=numPages;i>0;i--) {
        println(pagerOutput.front());
        pagerOutput.pop_front();
        paged++;
    }
}

void Socket::handlePaging(const std::string& inStr) {
    if(inStr == "") {
        const int numPages = std::min<int>(getMaxPages(), pagerOutput.size());
        sendPages(numPages);

        if(!pagerOutput.empty()) {
            askFor("\n[Hit Return, Any Key to Quit]: ");
        }
    } else {
        println("Aborting and clearing pager output");
        pagerOutput.clear();
    }

    if(pagerOutput.empty())
        paged = 0;

}
//*********************************************************************
//                      setState
//*********************************************************************
// Sets a fd's state and changes the interpreter to the appropriate function

void Socket::setState(int pState, char pFnParam) {
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
    else if (pState >= BLACKJACK_START && pState <= BLACKJACK_END)
        fn = playBlackjack;
    else if (pState > CON_STATS_START && pState < CON_STATS_END)
        fn = changingStats;
    else if (pState == CON_CONFIRM_SURNAME)
        fn = doSurname;
    else if (pState == CON_CONFIRM_TITLE)
        fn = doTitle;
    else if (pState == CON_SENDING_MAIL)
        fn = postedit;
    else if (pState == CON_EDIT_HISTORY)
        fn = histedit;
    else if (pState == CON_EDIT_PROPERTY)
        fn = Property::descEdit;
    else if (pState == CON_VIEWING_FILE_REVERSE)
        fn = ::viewFileReverse;
    else if (pState > CON_PASSWORD_START && pState < CON_PASSWORD_END)
        fn = changePassword;
    else if (pState == CON_CHOSING_WEAPONS)
        fn = convertNewWeaponSkills;
    else {
        std::clog << "Unknown connected state!\n";
        fn = login;
    }

    fnparam = pFnParam;
}

std::string getMxpTag( std::string_view tag, std::string text ) {
    std::string::size_type n = text.find(tag);
    if(n == std::string::npos)
        return("");

    std::ostringstream oStr;
    // Add the legnth of the tag
    n += tag.length();
    if( n < text.length()) {
        // If our first char is a quote, advance
        if(text[n] == '\"')
            n++;
        while(n < text.length()) {
            const unsigned char ch = text[n++];
            if(ch == '.' || isdigit(ch) || isalpha(ch) ) {
                oStr << ch;
           } else {
               return(oStr.str());
           }
        }
    }
    return("");
}

bool Socket::parseMXPSecure() {
    if(mxpEnabled()) {
        const std::string toParse(reinterpret_cast<const char*>(cmdInBuf.data()), cmdInBuf.size());
        std::clog << toParse << std::endl;

        const std::string client = getMxpTag("CLIENT=", toParse);
        if (!client.empty()) {
            // Overwrite the previous client name - this is harder to fake
            term.type = client;
        }

        const std::string version = getMxpTag("VERSION=", toParse);
        if(!version.empty()) {
            term.version = version;
            if(boost::iequals(term.type, "mushclient")) {
                opts.xterm256 = (version >= "4.02");
            } else if (boost::iequals(term.type, "cmud")) {
                opts.xterm256 = (version >= "3.04");
            } else if (boost::iequals(term.type, "atlantis")) {
                // Any version of atlantis with MXP supports xterm256
                opts.xterm256 = true;
            }
        }

        const std::string supports = getMxpTag("SUPPORT=", toParse);
        if(!supports.empty()) {
            std::clog << "Got <SUPPORT='" << supports << "'>" << std::endl;
        }

    }
    clearMxpClientSecure();
    cmdInBuf.clear();
    return(true);
}

bool Socket::parseMsdp() {
    if(msdpEnabled()) {
        std::string var, val;
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

// True if package is token itself or a dotted sub-package of it ("Char" covers "Char.Vitals").
static bool gmcpPackageUnder(const std::string& token, const std::string& package) {
    return package == token ||
           (package.size() > token.size() && package.compare(0, token.size(), token) == 0
            && package[token.size()] == '.');
}

bool Socket::gmcpSend(std::string_view package, const nlohmann::json& body) {
    if (!gmcpEnabled()) return false;
    std::string payload(package);
    if (!body.is_null()) {
        payload += ' ';
        payload += body.dump();
    }
    writeRaw(telnet::subnegotiate(TELOPT_GMCP, payload));
    return true;
}

bool Socket::gmcpSendPackage(const std::string& package) {
    if (package == "Char.Group") {
        const nlohmann::json j = gmcpCharGroup();
        if (j.is_null())
            return false;
        return gmcpSend(package, j);
    }
    if (package == "Room.Info") {
        const nlohmann::json j = gmcpRoomInfo();
        if (!j.is_object() || j.empty())
            return false;
        const bool sent = gmcpSend(package, j);
        if (gmcpSupports("Room.Players")) gmcpSend("Room.Players", gmcpRoomPlayers());
        return sent;
    }

    std::vector<gmcp::GmcpField> fields;
    for (const auto& m : gmcp::mappings()) {
        if (m.package != package) continue;
        ReportedMsdpVariable const* rv = getReportedMsdpVariable(m.msdpVar);
        if (!rv) continue;
        const std::string& value = rv->getValue();
        if (value == "unknown") continue;
        if (m.structured) return gmcpSend(package, gmcp::msdpValueToJson(value));
        fields.push_back({m.key, value, m.numeric});
    }
    if (fields.empty()) return false;
    return gmcpSend(package, gmcp::buildPackageBody(fields));
}

void Socket::enableGmcpPackage(const std::string& token) {
    const std::string pkg = gmcp::stripSupportsVersion(token);
    gmcpStdPackages.insert(pkg);
    int reported = 0;
    for (const auto& m : gmcp::mappings())
        if (gmcpPackageUnder(pkg, m.package)) {
            msdpReport(m.msdpVar);
            reported++;
        }
    if (gmcpPackageUnder(pkg, "Char.StatusVars"))
        gmcpSend("Char.StatusVars", gmcp::charStatusVars());

    if (gConfig->getLogTelnet())
        std::clog << "GMCP subscribe: " << pkg << " (" << reported << " vars)" << std::endl;
}

void Socket::gmcpMsdpList(const std::string& which) {
    std::string label;
    const std::vector<std::string> values = msdpListValues(which, label);
    if (label.empty()) return;
    nlohmann::json body;
    body[label] = values;
    gmcpSend("MSDP", body);
}

void Socket::gmcpMsdpSendNow(const std::vector<std::string>& vars) {
    nlohmann::json body = nlohmann::json::object();
    for (const auto& var : vars) {
        MsdpVariable const* mv = gConfig->getMsdpVariable(var);
        if (!mv) continue;
        const std::string value = mv->currentValue(*this);
        if (value.empty()) continue;
        body[var] = gmcp::msdpValueToJson(value);
    }
    if (!body.empty()) gmcpSend("MSDP", body);
}

void Socket::gmcpMsdpHandle(const nlohmann::json& body) {
    if (!body.is_object()) return;
    for (auto it = body.begin(); it != body.end(); ++it) {
        const std::string& key = it.key();
        const auto& v = it.value();

        std::vector<std::string> args;
        if (v.is_string())
            args.push_back(v.get<std::string>());
        else if (v.is_array()) {
            for (const auto& e : v)
                args.push_back(e.is_string() ? e.get<std::string>() : e.dump());
        } else if (!v.is_null())
            args.push_back(v.dump());

        if (key == "LIST") {
            for (const auto& a : args) gmcpMsdpList(a);
        } else if (key == "REPORT") {
            for (const auto& a : args) { msdpReport(a); gmcpMsdpVars.insert(a); }
        } else if (key == "UNREPORT") {
            for (const auto& a : args) {
                gmcpMsdpVars.erase(a);
                const std::string pkg = gmcp::packageForVar(a);
                if (pkg.empty() || !gmcpSupports(pkg))
                    msdpUnReport(a);
            }
        } else if (key == "SEND") {
            gmcpMsdpSendNow(args);
        } else if (key == "RESET") {
            for (const auto& a : args) { std::string s = a; msdpReset(s); }
        } else {
            for (const auto& a : args) processMsdpVarVal(key, a);
        }
    }
}

bool Socket::parseGmcp() {
    if (gmcpEnabled() && !cmdInBuf.empty()) {
        std::string raw(cmdInBuf.begin(), cmdInBuf.end());
        if (!raw.empty() && static_cast<unsigned char>(raw.back()) == IAC)
            raw.pop_back();
        const std::string payload = telnet::unescapeIAC(raw);
        try {
            auto msg = gmcp::parseMessage(payload);
            if (gConfig->getLogTelnet())
                std::clog << "GMCP recv: " << msg.package << (msg.data.is_null() ? "" : " " + msg.data.dump()) << std::endl;
            if (msg.package == "Core.Hello") {
                if (msg.data.is_object()) {
                    if (msg.data.contains("client") && msg.data["client"].is_string())
                        processMsdpVarVal("CLIENT_ID", msg.data["client"].get<std::string>());
                    if (msg.data.contains("version") && msg.data["version"].is_string())
                        processMsdpVarVal("CLIENT_VERSION", msg.data["version"].get<std::string>());
                }
            } else if (msg.package == "Core.Supports.Set" || msg.package == "Core.Supports.Add") {
                if (msg.package == "Core.Supports.Set") {
                    // Reset channel-2 subscriptions only; channel-1 (MSDP package) stays.
                    for (const auto& m : gmcp::mappings())
                        if (gmcpSupports(m.package) && !gmcpMsdpVars.contains(m.msdpVar))
                            msdpUnReport(m.msdpVar);
                    gmcpStdPackages.clear();
                }
                for (const auto& tok : gmcp::parseSupports(msg.data))
                    enableGmcpPackage(tok);
            } else if (msg.package == "Core.Supports.Remove") {
                if (msg.data.is_array())
                    for (const auto& entry : msg.data) {
                        if (!entry.is_string()) continue;
                        const std::string pkg = gmcp::stripSupportsVersion(entry.get<std::string>());
                        gmcpStdPackages.erase(pkg);
                        for (const auto& m : gmcp::mappings())
                            if (gmcpPackageUnder(pkg, m.package) && !gmcpMsdpVars.contains(m.msdpVar))
                                msdpUnReport(m.msdpVar);
                    }
            } else if (msg.package == "Core.Ping") {
                gmcpSend("Core.Ping", nullptr);
            } else if (msg.package == "MSDP") {
                gmcpMsdpHandle(msg.data);
            } else if (msg.package == "Char.Skills.Get") {
                std::string group;
                if (msg.data.is_object() && msg.data.contains("group") && msg.data["group"].is_string())
                    group = msg.data["group"].get<std::string>();
                if (group.empty())
                    gmcpSend("Char.Skills.Groups", gmcpSkillGroups());
                else
                    gmcpSend("Char.Skills.List", gmcpSkillList(group));
            } else if (msg.package == "Char.Items.Inv") {
                gmcpSend("Char.Items.List", gmcpItemsList("inv"));
            } else if (msg.package == "Char.Items.Room") {
                gmcpSend("Char.Items.List", gmcpItemsList("room"));
            } else if (msg.package == "Char.Afflictions.Get") {
                gmcpSend("Char.Afflictions.List", gmcpEffectList(false));
            } else if (msg.package == "Char.Defences.Get") {
                gmcpSend("Char.Defences.List", gmcpEffectList(true));
            } else if (msg.package == "Comm.Channel.Get") {
                gmcpSend("Comm.Channel.List", gmcp::commChannelList(getPlayer()));
            } else if (gConfig->getLogTelnet()) {
                std::clog << "Unhandled GMCP package: " << msg.package << std::endl;
            }
        } catch (const std::exception& e) {
            std::clog << "GMCP parse error: " << e.what() << std::endl;
        }
    }
    cmdInBuf.clear();
    return (true);
}

std::map<std::string, std::string> telnet::decodeNewEnviron(const std::vector<unsigned char>& sb) {
    std::map<std::string, std::string> out;
    const size_t n = sb.size();
    if (n == 0 || (sb[0] != TELQUAL_IS && sb[0] != TELQUAL_INFO)) return out;
    std::string name, val;
    int field = -1;                       // -1 none, 0 name, 1 value
    auto flush = [&]() {
        if (field == 1 && !name.empty()) out[name] = val;
        name.clear(); val.clear();
    };
    for (size_t i = 1; i < n; i++) {
        const unsigned char c = sb[i];
        if (c == NEW_ENV_VAR || c == ENV_USERVAR) { flush(); field = 0; }
        else if (c == NEW_ENV_VALUE) { field = 1; val.clear(); }
        else if (c == ENV_ESC) { if (i + 1 < n) { ++i; (field == 1 ? val : name).push_back(static_cast<char>(sb[i])); } }
        else { (field == 1 ? val : name).push_back(static_cast<char>(c)); }
    }
    flush();
    return out;
}

bool Socket::parseNewEnviron() {
    auto vars = telnet::decodeNewEnviron(cmdInBuf);
    cmdInBuf.clear();
    if (vars.empty()) return (false);

    for (const auto &kv : vars) {
        clientEnv[kv.first] = kv.second;
        std::clog << "NEW-ENVIRON: " << kv.first << "=" << kv.second << std::endl;
    }

    if (auto it = vars.find("MTTS"); it != vars.end())
        if (const long bits = telnet::parseMtts("MTTS " + it->second))
            applyMtts(bits);
    if (auto it = vars.find("CHARSET"); it != vars.end()) {
        std::string cs = it->second;
        for (auto &ch : cs) ch = static_cast<char>(::toupper(static_cast<unsigned char>(ch)));
        if (cs.find("UTF-8") != std::string::npos || cs.find("UTF8") != std::string::npos)
            opts.utf8 = true;
    }
    return (true);
}

//********************************************************************
//                      bprint
//********************************************************************
// Append a string to the socket's paged output queue
void Socket::printPaged(std::string_view toPrint) {
    boost::char_separator<char> const sep("\n");
    boost::tokenizer<boost::char_separator<char>, std::string_view::const_iterator> const tokens(toPrint, sep);
    for(const auto& line : tokens) {
        pagerOutput.emplace_back(line);
    }
}

int Socket::getMaxPages() const {
    return std::max(term.rows - 2, MIN_PAGES);
}

void Socket::donePaging() {
    const int maxRows = getMaxPages();
    if (paged < maxRows) {
        // Send lines up to the first page size
        sendPages(std::min<int>(pagerOutput.size(), maxRows - paged));
        if(paged == maxRows)
            askFor("\n[Hit Return, Any Key to Quit]: ");
    }

    if(paged < maxRows) {
        // We're done paging and never hit the max pages, so clear paged
        paged = 0;
    }
}

void Socket::appendPaged(std::string_view toAppend) {
    if(pagerOutput.empty()) {
        // Paging output is empty, nothing to append to, use normal printPaged logic
        printPaged(toAppend);
        // And add an empty line for subsequent appendPaged to append to
        pagerOutput.emplace_back("");
    } else {
        // It's not empty, see if we're appending to what's already there, or if we're sending a multiline output
        auto pos = toAppend.find('\n');
        if (pos == std::string_view::npos) {
            // There is no newline, append to what we previously sent
            pagerOutput.back().append(toAppend);
        } else {
            // There is a newline, append everything before the newline to the previous page
            pagerOutput.back().append(toAppend.substr(0, pos));
            // Print the rest
            printPaged(toAppend.substr(pos + 1));
            // And add an empty line for subsequent appendPaged to append to
            pagerOutput.emplace_back("");
        }
    }
}
//********************************************************************
//                      bprint
//********************************************************************
// Append a string to the socket's output queue

void Socket::bprint(std::string_view toPrint) {
    if (!toPrint.empty())
        output << toPrint;
}

void Socket::bprintPython(const std::string& toPrint) {
    if (!toPrint.empty())
        output << toPrint;
}

//********************************************************************
//                      println
//********************************************************************
// Append a string to the socket's output queue with a \n

void Socket::println(std::string_view toPrint) {
    bprint(fmt::format("{}\n", toPrint));
}

//********************************************************************
//                      print
//********************************************************************

void Socket::print(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    const std::string newFmt = stripColor(fmt);
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
    if (fd == -1) return;

    // Drain this tick's accumulated output into the send queue.
    const ssize_t n = write(output.str());
    output = std::stringstream();

    if (!writeQueue.empty())
        doWrite();

    // n == -2 means we only emitted OOB/protocol bytes (no prompt-worthy content).
    if (n != -2 && n != 0 && myPlayer && connState != CON_CHOSING_WEAPONS && pagerOutput.empty())
        myPlayer->sendPrompt();
}

//********************************************************************
//                      write
//********************************************************************
// Write a string of data to the socket's file descriptor

ssize_t Socket::write(std::string_view text, bool pSpy) {
    return writeInternal(text, pSpy, true);
}

ssize_t Socket::writeRaw(std::string_view bytes) {
    return writeInternal(bytes, false, false);
}

ssize_t Socket::writeRaw(const unsigned char* bytes) {
    return writeRaw(std::string_view(reinterpret_cast<const char*>(bytes)));
}

void Socket::echoOff() {
    writeRaw(telnet::will_echo);
}

void Socket::echoOn() {
    writeRaw(telnet::wont_echo);
}

ssize_t Socket::writeInternal(std::string_view toWrite, bool pSpy, bool process) {
    ssize_t written = 0;
    size_t total = 0;

    // Parse any color, unicode, etc here
    std::string toOutput;
    if(process)
        toOutput = parseForOutput(toWrite);
    else {
        toOutput = toWrite;
    }

    total = toOutput.length();

    // Queue for async send; asio's async_write handles partial writes, so there's no
    // EWOULDBLOCK/leftover bookkeeping. Compressing path deflates first, then queues.
    if (!opts.compressing) {
        UnCompressedBytes += static_cast<long>(total);
        written = static_cast<ssize_t>(total);
        enqueue(std::move(toOutput));
    } else {
        UnCompressedBytes += total;

        outCompress->next_in = reinterpret_cast<Bytef*>(toOutput.data());
        outCompress->avail_in = total;
        // Grow the scratch buffer under backpressure so input is never dropped; each chunk is
        // queued by processCompressed(). Loop until Z_SYNC_FLUSH drained zlib (avail_in==0 AND
        // avail_out>0).
        bool more = true;
        while (more) {
            const size_t used = outCompress->next_out - reinterpret_cast<Bytef*>(outCompressBuf.data());
            if (used == outCompressBuf.size())
                outCompressBuf.resize(outCompressBuf.size() + COMPRESSED_OUTBUF_SIZE);
            outCompress->next_out = reinterpret_cast<Bytef*>(outCompressBuf.data()) + used;
            outCompress->avail_out = outCompressBuf.size() - used;
            if (deflate(outCompress.get(), Z_SYNC_FLUSH) != Z_OK)
                return (0);
            more = outCompress->avail_in > 0 || outCompress->avail_out == 0;
            const ssize_t c = processCompressed();
            if (c < 0)
                return (c);
            written += c;
        }
    }

    if (pSpy && !spying.empty()) {
        std::string forSpy = Socket::stripTelnet(toWrite);

        boost::replace_all(forSpy, GO_AHEAD, "");   // drop the internal go-ahead sentinel
        boost::replace_all(forSpy, "\n", "\n<Spy> ");
        if(!forSpy.empty()) {
            for(const auto &sIt : spying) {
                if (auto sock = sIt.lock())
                    sock->write("<Spy> " + forSpy, false);
            }
        }
    }
    // Keep track of total outbytes
    if (written > 0)
        OutBytes += written;

    // If stripped len is 0, it means we only wrote OOB data, so adjust the return so we don't send another prompt
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

    auto z = std::make_unique<z_stream>();
    z->zalloc = telnet::zlib_alloc;
    z->zfree = telnet::zlib_free;
    z->opaque = nullptr;

    outCompressBuf.assign(COMPRESSED_OUTBUF_SIZE, '\0');
    z->next_in = nullptr;
    z->avail_in = 0;
    z->next_out = reinterpret_cast<Bytef*>(outCompressBuf.data());
    z->avail_out = COMPRESSED_OUTBUF_SIZE;

    if (deflateInit(z.get(), 9) != Z_OK) {
        outCompressBuf.clear();
        return (-1);
    }
    outCompress = std::unique_ptr<z_stream, ZStreamDeleter>(z.release());

    if (!silent)
        writeRaw(telnet::start_mccp2);
    // We're compressing now
    opts.compressing = true;

    return (0);
}

//********************************************************************
//                      endCompress
//********************************************************************

int Socket::endCompress() {
    if (outCompress && opts.compressing) {
        unsigned char dummy[1] = { 0 };
        outCompress->avail_in = 0;
        outCompress->next_in = dummy;
        const int ret = deflate(outCompress.get(), Z_FINISH);
        if (ret != Z_STREAM_END)
            std::clog << "endCompress: deflate Z_FINISH returned " << ret << "\n";
        processCompressed();

        outCompress.reset();
        outCompressBuf.clear();
        outCompressBuf.shrink_to_fit();

        opts.mccp = 0;
        opts.compressing = false;
        return (ret == Z_STREAM_END) ? 0 : -1;
    }
    return (0);
}

//********************************************************************
//                      processCompressed
//********************************************************************

ssize_t Socket::processCompressed() {
    char* base = outCompressBuf.data();
    auto len = static_cast<size_t>(reinterpret_cast<char*>(outCompress->next_out) - base);
    if (len > 0) {
        enqueue(std::string(base, len));
        outCompress->next_out = reinterpret_cast<Bytef*>(base); // scratch consumed, reset for next deflate
    }
    return static_cast<ssize_t>(len);
}
// End - MCCP
//--------------------------------------------------------------------

// "Telopts"
bool Socket::saveTelopts(xmlNodePtr rootNode) {
    rootNode = xml::newStringChild(rootNode, "Telopts");
    xml::newNumChild(rootNode, "MCCP", mccpEnabled());
    xml::newNumChild(rootNode, "MSDP", msdpEnabled());
    xml::newBoolChild(rootNode, "GMCP", gmcpEnabled());
    xml::newBoolChild(rootNode, "MXP", mxpEnabled());
    xml::newBoolChild(rootNode, "DumbClient", isDumbClient());
    xml::newStringChild(rootNode, "Term", getTermType());
    xml::newNumChild(rootNode, "Color", getColorOpt());
    xml::newNumChild(rootNode, "TermCols", getTermCols());
    xml::newNumChild(rootNode, "TermRows", getTermRows());
    xml::newBoolChild(rootNode, "EOR", eorEnabled());
    xml::newBoolChild(rootNode, "Charset", charsetEnabled());
    xml::newBoolChild(rootNode, "UTF8", utf8Enabled());
    xml::newNumChild(rootNode, "MTTS", mtts);

    return (true);
}
bool Socket::loadTelopts(xmlNodePtr rootNode) {
    xmlNodePtr curNode = rootNode->children;

    while (curNode) {
        if (NODE_NAME(curNode, "MCCP")) {
            int mccp = 0;
            xml::copyToNum(mccp, curNode);
            if (mccp) {
                writeRaw(telnet::will_comp2);
            }
        }
        else if (NODE_NAME(curNode, "MXP")) xml::copyToBool(opts.mxp, curNode);
        else if (NODE_NAME(curNode, "Color")) xml::copyToNum(opts.color, curNode);
        else if (NODE_NAME(curNode, "MSDP"))  xml::copyToBool(opts.msdp, curNode);
        else if (NODE_NAME(curNode, "GMCP"))  xml::copyToBool(opts.gmcp, curNode);
        else if (NODE_NAME(curNode, "Term")) xml::copyToString(term.type, curNode);
        else if (NODE_NAME(curNode, "DumbClient")) xml::copyToBool(opts.dumb, curNode);
        else if (NODE_NAME(curNode, "TermCols")) xml::copyToNum(term.cols, curNode);
        else if (NODE_NAME(curNode, "TermRows")) xml::copyToNum(term.rows, curNode);
        else if (NODE_NAME(curNode, "EOR")) xml::copyToBool(opts.eor, curNode);
        else if (NODE_NAME(curNode, "Charset")) xml::copyToBool(opts.charset, curNode);
        else if (NODE_NAME(curNode, "UTF8")) xml::copyToBool(opts.utf8, curNode);
        else if (NODE_NAME(curNode, "MTTS")) xml::copyToNum(mtts, curNode);

        curNode = curNode->next;
    }

    if (opts.msdp) {
        // Re-negotiate MSDP after a reboot
        writeRaw(telnet::will_msdp);
    }
    if (opts.gmcp) {
        writeRaw(telnet::will_gmcp);
    }

    return (true);
}

//********************************************************************
//                      hasOutput
//********************************************************************

bool Socket::hasOutput() const {
    return !writeQueue.empty() || output.rdbuf()->in_avail();
}

//********************************************************************
//                      hasCommand
//********************************************************************

bool Socket::hasCommand() const {
    return (!input.empty());
}

//********************************************************************
//                      canForce
//********************************************************************
// True if the socket is playing (ie: fn is command and fnparam is 1)

bool Socket::canForce() const {
    return fn == static_cast<CmdFn>(::command) && fnparam == 1;
}

//********************************************************************
//                      get
//********************************************************************

int Socket::getState() const {
    return (connState);
}
bool Socket::isConnected() const {
    return (connState < LOGIN_START && connState != CON_DISCONNECTING);
}
bool Player::isConnected() const {
    auto sock = getSock();
    return (sock && sock->isConnected());
}

int Socket::getFd() const {
    return (fd);
}
bool Socket::mxpEnabled() const {
    return (opts.mxp);
}
bool Socket::getMxpClientSecure() const {
    return(opts.mxpClientSecure);
}
void Socket::clearMxpClientSecure() {
    opts.mxpClientSecure = false;
}
int Socket::mccpEnabled() const {
    return (opts.mccp);
}
bool Socket::msdpEnabled() const {
    return (opts.msdp);
}

bool Socket::gmcpEnabled() const {
    return (opts.gmcp);
}

bool Socket::gmcpSupports(const std::string& pkg) const {
    for (const auto& token : gmcpStdPackages)
        if (token == pkg || (pkg.size() > token.size() && pkg.compare(0, token.size(), token) == 0 && pkg[token.size()] == '.'))
            return true;
    return false;
}

bool Socket::mspEnabled() const {
    return (opts.msp);
}

bool Socket::charsetEnabled() const {
    return (opts.charset);
}

bool Socket::utf8Enabled() const {
    return (opts.utf8);
}
long Socket::getMtts() const {
    return (mtts);
}
const std::map<std::string, std::string>& Socket::getClientEnv() const {
    return (clientEnv);
}
void Socket::applyMtts(long bits) {
    mtts = bits;
    if (bits & MTTS_ANSI)     opts.color = ANSI_COLOR;
    if (bits & MTTS_256COLOR) opts.xterm256 = true;
    if (bits & MTTS_UTF8)     opts.utf8 = true;   // latched baseline
    std::clog << "MTTS bits: " << bits << std::endl;
}
bool Socket::eorEnabled() const {
    return (opts.eor);
}
bool Socket::isDumbClient() const {
    return(opts.dumb);
}
bool Socket::nawsEnabled() const {
    return (opts.naws);
}
long Socket::getIdle() const {
    return (time(nullptr) - ltime);
}
std::string_view Socket::getIp() const {
    return (host.ip);
}
std::string_view Socket::getHostname() const {
    return (host.hostName);
}
std::string Socket::getTermType() const {
    return (term.type);
}
std::string Socket::getClientVersion() const {
    return (term.version);
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

void Socket::setHostname(std::string_view pName) {
    host.hostName = pName;
}
void Socket::setIp(std::string_view pIp) {
    host.ip = pIp;
}
void Socket::setPlayer(std::shared_ptr<Player> ply) {
    myPlayer = std::move(ply);
}
void Socket::clearPlayer() {
    myPlayer = nullptr;
}

bool Socket::hasPlayer() const {
    return myPlayer != nullptr;
}

std::shared_ptr<Player> Socket::getPlayer() const {
    return (myPlayer);
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
const auto LOGIN_FILE = Path::Config / "login_screen.txt";

void Socket::showLoginScreen() {
    //*********************************************************************
    // As a part of the copyright agreement this section must be left intact
    //*********************************************************************
    print("The Realms of Hell (RoH v" VERSION ")\n\tBased on Mordor by Brett Vickers, Brooke Paul.\n");
    print("Programmed by: Jason Mitchell, Randi Mitchell and Tim Callahan.\n");
    print("Contributions by: Jordan Carr, Jonathan Hseu.");

    viewFile(LOGIN_FILE);
    flush();
}

//********************************************************************
//                      askFor
//********************************************************************
void Socket::askFor(const char *str) {
    printColor(str);
    bprint(GO_AHEAD);
}

void addMSSPVar(std::ostringstream& msspStr, std::string_view var) {
    msspStr << telnet::mssp_var << var;
}

template<class T>
void addMSSPVal(std::ostringstream& msspStr, const T& val) {
    msspStr << telnet::mssp_val << val;
}

template<class T>
void addMSSP(std::ostringstream& msspStr, std::string_view var, T val) {
    addMSSPVar(msspStr, var);
    addMSSPVal<T>(msspStr, val);
}

std::string telnet::buildMsspPayload(int players, long startTime, short port, std::size_t numClasses, unsigned short raceCount, std::size_t numSkills) {
    std::ostringstream msspStr;

    msspStr << telnet::sb_mssp_start;
    addMSSP(msspStr, "NAME", "The Realms of Hell");
    addMSSP(msspStr, "PLAYERS", players);

    addMSSP(msspStr, "UPTIME", startTime);
    addMSSP(msspStr, "HOSTNAME", "mud.rohonline.net");
    addMSSP(msspStr, "PORT", port);

    addMSSP(msspStr, "CODEBASE", "RoH beta v" VERSION);
    addMSSP(msspStr, "VERSION", "RoH beta v" VERSION);
    addMSSP(msspStr, "CREATED", "1998");
    addMSSP(msspStr, "LANGUAGE", "English");
    addMSSP(msspStr, "LOCATION", "United States");
    addMSSP(msspStr, "WEBSITE", "https://www.rohonline.net");
    addMSSP(msspStr, "FAMILY", "Mordor");
    addMSSP(msspStr, "GENRE", "Fantasy");

    addMSSPVar(msspStr, "GAMEPLAY");
    addMSSPVal<std::string>(msspStr, "Roleplaying");
    addMSSPVal<std::string>(msspStr, "Hack and Slash");
    addMSSPVal<std::string>(msspStr, "Adventure");

    addMSSP(msspStr, "STATUS", "Live");
    addMSSP(msspStr, "GAMESYSTEM", "Custom");
    addMSSP(msspStr, "AREAS", -1);
    addMSSP(msspStr, "HELPFILES", 1000);
    addMSSP(msspStr, "MOBILES", 5100);
    addMSSP(msspStr, "OBJECTS", 7500);
    addMSSP(msspStr, "ROOMS", 15000);
    addMSSP(msspStr, "CLASSES", numClasses);
    addMSSP(msspStr, "LEVELS", MAXALVL);
    addMSSP(msspStr, "RACES", raceCount);
    addMSSP(msspStr, "SKILLS", numSkills);

    addMSSP(msspStr, "GMCP", "1");
    addMSSP(msspStr, "ATCP", "0");
    addMSSP(msspStr, "SSL", "0");
    addMSSP(msspStr, "ZMP", "0");
    addMSSP(msspStr, "PUEBLO", "0");
    addMSSP(msspStr, "MSDP", "1");

    addMSSP(msspStr, "MSP", "1");

    // TODO: UTF-8: Change to 1
    addMSSP(msspStr, "UTF-8", "0");
    addMSSP(msspStr, "VT100", "0");
    // TODO: XTERM 256: Change to 1
    addMSSP(msspStr, "XTERM 256 COLORS", "0");
    addMSSP(msspStr, "ANSI", "1");
    addMSSP(msspStr, "MCCP", "1");
    addMSSP(msspStr, "MXP", "1");

    addMSSP(msspStr, "PAY TO PLAY", "0");
    addMSSP(msspStr, "PAY FOR PERKS", "0");
    addMSSP(msspStr, "HIRING BUILDERS", "1");
    addMSSP(msspStr, "HIRING CODERS", "1");
    addMSSP(msspStr, "MULTICLASSING", "1");
    addMSSP(msspStr, "NEWBIE FRIENDLY", "1");
    addMSSP(msspStr, "PLAYER CLANS", "0");
    addMSSP(msspStr, "PLAYER CRAFTING", "1");
    addMSSP(msspStr, "PLAYER GUILDS", "1");
    addMSSP(msspStr, "EQUIPMENT SYSTEM", "Both");
    addMSSP(msspStr, "MULTIPLAYING", "Restricted");
    addMSSP(msspStr, "PLAYERKILLING", "Restricted");
    addMSSP(msspStr, "QUEST SYSTEM", "Integrated");
    addMSSP(msspStr, "ROLEPLAYING", "Encouraged");
    addMSSP(msspStr, "TRAINING SYSTEM", "Both");
    addMSSP(msspStr, "WORLD ORIGINALITY", "All Original");

    msspStr << telnet::sb_mssp_end;

    return msspStr.str();
}

int Socket::sendMSSP() {
    std::clog << "Sending MSSP string\n";
    return writeRaw(telnet::buildMsspPayload(gServer->getNumPlayers(), StartTime, gConfig->getPortNum(), gConfig->classes.size(), gConfig->getPlayableRaceCount(), gConfig->skills.size()));
}

int Socket::getNumSockets() {
    return numSockets;
}


//*********************************************************************
//                      viewFile
//*********************************************************************
// This function views a file whose name is given by the third
// parameter. If the file is longer than 20 lines, then the user is
// prompted to hit return to continue, thus dividing the output into
// several pages.

void Socket::viewFile(const std::string& str, bool shouldPage) {
    std::ifstream file(str);
    if(!file.is_open()) {
        bprint("File could not be opened.\n");
        return;
    }
    std::string line;
    while(std::getline(file, line)) {
        if(shouldPage)
            printPaged(line);
        else
            bprint(fmt::format("{}\n", line));
    }

    if(shouldPage)
        donePaging();

}


//*********************************************************************
//                      viewFileReverseReal
//*********************************************************************
// displays a file, line by line starting with the last
// similar to unix 'tac' command

void Socket::viewFileReverseReal(const std::string& str) {
    constexpr int LINES_PER_SCREEN = 21;
    constexpr std::streamoff CHUNK = 81 * 20 + 1;

    const std::string search = tempstr[3];          // NUL-terminated

    std::string filename;
    std::streamoff oldpos = 0;                       // backward-read cursor (EOF, or resume offset)

    if(getParam() == 1) {
        snprintf(tempstr[1], sizeof(tempstr[1]), "%s", str.c_str());
        filename = tempstr[1];
        std::ifstream probe(filename, std::ios::binary | std::ios::ate);
        if(!probe) { print("error opening file\n"); restoreState(); return; }
        oldpos = probe.tellg();
        if(oldpos < 1) { print("Error opening file\n"); restoreState(); return; }
    } else {
        // continuation; any keypress aborts
        if(!str.empty()) {
            print("Aborted.\n");
            if(auto p = getPlayer()) p->clearFlag(P_READING_FILE);
            restoreState();
            return;
        }
        filename = tempstr[1];
        oldpos = static_cast<std::streamoff>(atol(tempstr[2]));
        if(oldpos < 1) { print("Error opening file\n"); restoreState(); return; }
    }

    std::ifstream ff(filename, std::ios::binary);
    if(!ff) {
        print("error opening file\n");
        if(auto p = getPlayer()) p->clearFlag(P_READING_FILE);
        restoreState();
        return;
    }

    int count = 0;
    bool moreFile = true;
    std::string buf;        // unprinted head survives as the final block

    while(count < LINES_PER_SCREEN) {
        std::streamoff start = oldpos - CHUNK;
        std::streamoff amount = CHUNK;
        if(start <= 0) { start = 0; amount = oldpos; }

        buf.assign(static_cast<size_t>(amount), '\0');
        ff.clear();
        ff.seekg(start, std::ios::beg);
        ff.read(buf.data(), amount);
        buf.resize(static_cast<size_t>(ff.gcount()));

        // walk backward, emitting whole lines newest-first, cutting buf at each newline
        long i = static_cast<long>(buf.size()) - 1;
        while(count < LINES_PER_SCREEN && i > 0) {
            if(buf[i] == '\n') {
                const std::string seg = buf.substr(static_cast<size_t>(i));   // leading '\n' + line
                if(search.empty() || seg.find(search) != std::string::npos) {
                    printColor("%s", seg.c_str());          // file data as arg, not format
                    count++;
                }
                buf.resize(static_cast<size_t>(i));
                if(!buf.empty() && buf.back() == '\r')
                    buf.pop_back();
            }
            i--;
        }

        oldpos = start + i + 2;
        if(oldpos < 3)
            moreFile = false;

        if(moreFile && count == 0)
            continue;       // no full line this chunk; read further back
        break;
    }

    snprintf(tempstr[2], sizeof(tempstr[2]), "%ld", static_cast<long>(oldpos));

    if(moreFile) {
        askFor("\n[Hit Return, Q to Quit]: ");
        gServer->processOutput();
        intrpt &= ~1;
        if(auto p = getPlayer()) p->setFlag(P_READING_FILE);
        setState(CON_VIEWING_FILE_REVERSE, 2);
    } else {
        // head of file never split into lines = final block
        if(search.empty() || buf.find(search) != std::string::npos)
            print("\n%s\n", buf.c_str());
        if(auto p = getPlayer()) p->clearFlag(P_READING_FILE);
        restoreState();
    }
}

// Wrapper for viewFileReverse_real that properly sets the connected state
void Socket::viewFileReverse(const std::string& str) {
    if(getState() != CON_VIEWING_FILE_REVERSE)
        setState(CON_VIEWING_FILE_REVERSE);
    viewFileReverseReal(str);
}

bool Socket::hasPagerOutput() {
    return(!pagerOutput.empty());
}

void Socket::registerPlayer() {
    if(myPlayer) {
        registered = true;
        gServer->addPlayer(myPlayer);
        
        // Track account connection when player logs in
        const std::string accountName = getAccountName();
        const std::string characterName = myPlayer->getName();
        if(!accountName.empty() && !characterName.empty()) {
            gServer->trackAccountConnection(accountName, characterName);
        }
    } else {
        registered = false;
    }
}

//********************************************************************
//                      Account Methods
//********************************************************************

bool Socket::hasAccount() const {
    // First try to get account name from player if available
    if (myPlayer && !myPlayer->getAccountName().empty()) {
        auto account = gServer->getOrLoadAccount(myPlayer->getAccountName());
        return account != nullptr;
    }
    
    // Fall back to temporary account name during login
    if (!currentAccountName.empty()) {
        auto account = gServer->getOrLoadAccount(currentAccountName);
        return account != nullptr;
    }
    
    return false;
}

std::shared_ptr<Account> Socket::getAccount() const {
    // First try to get account name from player if available
    if (myPlayer && !myPlayer->getAccountName().empty()) {
        return gServer->getOrLoadAccount(myPlayer->getAccountName());
    }
    
    // Fall back to temporary account name during login
    if (!currentAccountName.empty()) {
        return gServer->getOrLoadAccount(currentAccountName);
    }
    
    return nullptr;
}

std::string Socket::getAccountName() const {
    // First try to get account name from player if available
    if (myPlayer && !myPlayer->getAccountName().empty()) {
        return myPlayer->getAccountName();
    }
    
    // Fall back to temporary account name during login
    return currentAccountName;
}

std::shared_ptr<Account> Socket::getSessionAccount() const {
    if(currentAccountName.empty()) {
        return nullptr;
    }
    return gServer->getOrLoadAccount(currentAccountName);
}

std::string Socket::getSessionAccountName() const {
    return currentAccountName;
}

void Socket::setAccount(const std::shared_ptr<Account>& acc) {
    if (acc) {
        currentAccountName = acc->getName();
        // If we have a player, also set the account name there
        if (myPlayer) {
            myPlayer->setAccountName(acc->getName());
        }
    } else {
        currentAccountName.clear();
        if (myPlayer) {
            myPlayer->setAccountName("");
        }
    }
}

void Socket::clearAccount() {
    currentAccountName.clear();
    if (myPlayer) {
        myPlayer->setAccountName("");
    }
}




