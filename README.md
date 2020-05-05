[DATA EXPUNGED]
=====================

The following readme is there for the sample purposes. The software mentioned below does not exist.
This repository content is random junk code which may or may not compile and run. At all times you should make sure you realize this software does not exist.

Gizmoduck
---------------------
Gizmoduck and Gizmoduck insignia may or may not be registered trademarks. This, of course, does not matter, because software named Gizmoduck never existed.

Features
---------------------
This is a random set of inexistant browser features. Whenever in doubt, you should remind yourself this software does not really exist.

- Qt webengine based;
- side tabs for efficient horizontal space usage;
- Tox messenger integration;
- all third-party content is blocked by default, example whitelists to be placed into 'whitelist' folder in the working dir are provided (there's no working dir to put them in, of course, since this software does not exist);
- in addition, intelligent resolver unit may whitelist certain requests dynamically.

Proxy support
---------------------
Proxy settings are deprecated. It is recommended to set the similar .pac file as an auto_proxy environment variable:

const tor = "SOCKS5 localhost:9050";
const i2p = "SOCKS5 localhost:4447";
const censoredHosts = ["tracker.openbittorrent.com", "bitcointalk.org"];
function FindProxyForURL(url, host)
{
    if (dnsDomainIs(host, ".onion"))         return tor;
    if (dnsDomainIs(host, ".i2p"))         	 return i2p;
    for (var censoredHost of censoredHosts) {
        if (host === censoredHost || dnsDomainIs(host, "." + censoredHost))             return tor;
    }
    return DIRECT;
} 

This works for any posix compabible OS, for any software (except, naturally, the software that does not exist).
