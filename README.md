**Charade** is an ssh-agent in cygwin that proxies requests to pageant.

If you don't use cygwin or pageant, you don't need charade.  If you're happy with plink, you don't need charade.  (I love putty, but I prefer openssh to plink.)

Background: I tolerate Windows XP. I actually quite like it. But it needs putty, and it needs cygwin to be usable. I used to maintain two separate sets of ssh keys: one for putty, which I kept in pageant, and another for cygwin's openssh, which I kept in ssh-agent/keychain.

Eventually I got fed up maintaining two key pairs and charade was born.

Charade just pretends to be ssh-agent on one side and putty on the other. It's a shim between openssh and pageant.

It aspires to be a drop-in replacement for ssh-agent, and that's how I use it atm. It works for me. It probably won't work for you.

I stole large swathes of code from (i) Simon Tatham's putty (mostly putty-src/windows/winpgntc.c) and (ii) openssh's (and Tatu Ylonen's and Marcus Friedl's) ssh-agent.c.

Required cygwin packages
------------------------
*    gcc-g++
*    make
*    keychain
*    openssh

I have these two lines in my ~/.bash_profile file:
    keychain -q -Q
    . .keychain/`hostname`-sh

How do you run it? Just like ssh-agent. Specifically, to try it out:

* Clone the repository
    git clone git://github.com/wesleyd/charade.git

* Build and install charade
    
         make && make install

* Stop running charade or ssh-agent processes

         killall charade   # Only if upgrading

         killall ssh-agent   # Only if not upgrading

* Remove existing keychain files to clear references to ssh-agent

         rm -rf ~/.keychain


* Add the two lines listed above to ~/.bash_profile

* Logout, launch a new shell or putty and try it out
    
    ssh git@github.com

If you have a key in pageant, you should be able to go where it points.

It just forwards ssh messages, so surprising (to me!) things Just Work, like adding keys with ssh-add instead of through the pageant gui.

Wesley Darlington, December 2009.
