# udp-accept-test
Following code is more or less successful example that
accept() call could be simulated using UDP.
The code works sometimes. It's totally expected, due to fact
that most of the internet claims this won't work.
Probably, I was able to trigger some kernel specific behavior.
I expect this code to behave differently on different machines.

## Dependencies
Simply none! Only dependency needed is meson/ninja.
To install meson localy you can use following command:

```Bash
$ pip install --upgrade meson
```

To install ninja

```Bash
$ sudo apt install ninja-build
```
or

```Bash
$ sudo dnf insatll ninja-build
```

## Building code
I assume usage of some decent modern compiler, gcc >= 8.4 is fine.

```Bash
$ meson setup build
$ cd build
$ ninja
```

or

```Bash
$ meson setup build
$ cd build
$ meson compile
```

## Flow
Code uses simple reactor pattern based on select(). It listens to incoming connections on port 10555.

When first packet arrives, `acceptor` socket receives first 4 bytes.
Then duplicates `dup()` acceptor socket and uses `connect()` on it
to setup remote endpoint.

Next batch of data should be read through `int handler_read(int sock)`.
It uses standard recv() and send() to exchange data with remote endpoint.

## Behavior

Server correctly accepts incoming connection.
It knows origi point and creates client.
Unfortunately, first packet comes from 0.0.0.0:0.
Server reads only 4 bytes, remaining part of the packet is lost.
Why, I have no idea yet.

Remaining packets comes with correct endpoint values (host:port).
However, afer few packet, server "re-connects". So, something strange is
happening in the stack. That mentioned, this isn't unexpeded due to nature
of this experiment.

Session log from server looks like that:

```
[woba@woba1 udp-accept-test]$ cd build/
[woba@woba1 build]$ ninja
[2/2] Linking target udp-accept
[woba@woba1 build]$ ./udp-accept
Connecting from 0.0.0.0:0, tete
Connecting from 127.0.0.1:39880, jksl
reading client!!!
asdlksjadl

Connecting from 127.0.0.1:39880, asdk
reading client!!!
asjdlasd]

reading client!!!
asjdlksa

Connecting from 127.0.0.1:39880, asjd
reading client!!!
as

reading client!!!


reading client!!!


Connecting from 127.0.0.1:39880,

reading client!!!


reading client!!!


reading client!!!


reading client!!!
^CError: Interrupted system call
[woba@woba1 build]$
```

## Bugs
A lot! Code is not tested properly. This is merrly POC with happy coding
path.