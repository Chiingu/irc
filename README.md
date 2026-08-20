*This project has been created as part of the 42 curriculum by hbou-dou, nosahimi and mbouizak.*

# ft_irc

## Description

ft_irc is a single-process IRC server written in C++98. It accepts several TCP/IPv4
clients at the same time and implements the IRC registration, messaging, channel, and
channel-operator behavior required by the 42 subject.

The server uses one `poll()` call as its event multiplexer. Listening, client reads,
and client writes are all non-blocking and are performed only after the corresponding
descriptor is reported ready. Incoming bytes are retained until a complete IRC line
arrives, while outgoing lines are queued so that a slow client cannot block everyone
else.

Reference client: **irssi** on Ubuntu. The implementation also works with command-line
tools such as `nc` and with clients that begin registration using IRCv3 `CAP LS`.

## Features

- Password authentication with `PASS`, followed by `NICK` and `USER` registration.
- Channel membership and messaging with `JOIN`, `PART`, `PRIVMSG`, and `NOTICE`.
- Direct messages to nicknames and messages broadcast to channel peers.
- Channel operators and `KICK`, `INVITE`, `TOPIC`, and `MODE`.
- Required channel modes: `i` (invite-only), `t` (operator-only topic), `k` (key),
  `o` (operator status), and `l` (member limit).
- Client support commands including `CAP`, `PING`, `NAMES`, `LIST`, `WHO`, `WHOIS`,
  `MOTD`, `LUSERS`, `VERSION`, `AWAY`, and `QUIT`.
- Partial-command aggregation, multiple commands per packet, queued partial writes,
  disconnect cleanup, and automatic operator recovery when the last operator leaves.

### Bonus features

- **File transfer:** CTCP messages, including `DCC SEND` and `DCC ACCEPT`, are relayed
  unchanged through `PRIVMSG`. DCC-capable reference clients then establish their
  normal peer-to-peer file-transfer connection.
- **Bot:** send `PRIVMSG 42bot :help`, or use `!help`, `!ping`, `!about`, `!rules`, and
  `!dice` in a joined channel.

## Instructions

### Build

```sh
make
```

The Makefile uses `c++ -Wall -Wextra -Werror -std=c++98`, creates `ircserv`, tracks
header dependencies, and provides the required `all`, `clean`, `fclean`, and `re`
targets without unnecessary relinking.

### Run

```sh
./ircserv <port> <password>
```

Example:

```sh
./ircserv 6667 secret
```

Connect with irssi:

```sh
irssi -c 127.0.0.1 -p 6667 -w secret
```

Or connect with netcat on Ubuntu:

```sh
nc -C 127.0.0.1 6667
```

Then register manually:

```text
PASS secret
NICK alice
USER alice 0 * :Alice Example
JOIN #general
PRIVMSG #general :hello
```

IRC commands must end in `\r\n` or `\n`. A command may be split across several TCP
packets; the server processes it only when the terminating newline arrives.

```sh
make re CXX=c++
valgrind --leak-check=full --show-leak-kinds=all ./ircserv 6667 secret
```

In another terminal, connect clients and stop one with `Ctrl-Z`; flood its channel
from a second client, resume it with `fg`, and confirm queued messages arrive while
other clients remain responsive.

## Technical choices

- Client and channel objects have stable addresses and explicit ownership.
- Channel membership stores client pointers; disconnect cleanup removes those pointers
  before deleting the client.
- Each client has one receive accumulator and one bounded send queue.
- Nickname and channel lookups are case-insensitive.
- `SIGPIPE` is ignored so a peer disappearing during a write cannot terminate the
  process; `SIGINT` and `SIGTERM` leave the event loop and allow destructors to release
  sockets and allocated objects.

## Resources

- [RFC 2812 - Internet Relay Chat: Client Protocol](https://www.rfc-editor.org/rfc/rfc2812)
- [RFC 1459 - Internet Relay Chat Protocol](https://www.rfc-editor.org/rfc/rfc1459)
- [Linux poll(2)](https://man7.org/linux/man-pages/man2/poll.2.html)
- [Linux socket(7)](https://man7.org/linux/man-pages/man7/socket.7.html)
- [Modern IRC Client Protocol documentation](https://modern.ircdocs.horse/)
- [irssi documentation](https://irssi.org/documentation/)

AI was used to help turn the subject into a requirement checklist, 
review edge cases in IRC parsing and non-blocking I/O, draft repetitive protocol-test
scenarios, and review documentation.