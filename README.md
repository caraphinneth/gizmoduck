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
- tabs congesting for the same domain;
- Tox messenger integration;
- yt-dlp integration;
- all third-party content is blocked by default, example whitelists to be placed into 'whitelist' folder in the working dir are provided (there's no working dir to put them in, of course, since this software does not exist);
- in addition, intelligent resolver unit may whitelist certain requests dynamically.

Proxy support
---------------------
Proxy settings are deprecated. It is recommended to set the similar .pac file as an auto_proxy environment variable:

```
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
```

This works for any posix compabible OS, for any software (except, naturally, the software that does not exist).

Addendum as of 11.08.2024
---------------------
This is a set of issues that normally occur when migrating a project to Qt6.

- Icon display is mostly fixed. Loading icon, while looking fashionable, was of little help in the age of broadband with lazy-load, and only complicated the code unnecessary. As such, it does not exist;
- A bit of complication was required to handle popup windows, uplifting in progress;
- Experimental setting no longer accepted the default way. When in need of any, utilize `QTWEBENGINE_CHROMIUM_FLAGS` (e.g.: `export QTWEBENGINE_CHROMIUM_FLAGS="--ignore-gpu-blacklist"`).

Qt6 upgrade instructions
---------------------
Cover up or illuminate all sources of darkness in your location of shelter. Destroy any reflective surfaces in your location of shelter. Destroy any transparent surfaces in your location of shelter. If you begin witnessing bizarre, disturbing, or unsettling effects occuring in your location of shelter, do not report or point them out to any other person. Doing so will allow [CORRUPTED] to spread.

When you begin to hear the screaming in your mind, move to a room or place where you can be alone. Darken the room completely. This will allow the fading to happen most quickly. Bring any possessions of sentimental value with you. The screams will become overpowering. Do not panic. Focus on a memory of great intensity to you, and be silent until you do not exist.