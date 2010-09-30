#!/bin/sh

set -e

echo -n "Installing charade into /usr/bin ... "
install charade.exe /usr/bin
echo done

echo "Backing up old /usr/bin/ssh-agent.exe ..."
if [ -e /usr/bin/ssh-agent.exe ]; then
    if [ -f /usr/bin/ssh-agent-orig.exe ]; then
        echo -n "  Backup already exists: just removing old ssh-agent.exe ..."
        rm /usr/bin/ssh-agent.exe
        echo done
    else
        echo -n "  Moving ssh-agent.exe to ssh-agent-orig.exe ... "
        mv /usr/bin/ssh-agent.exe /usr/bin/ssh-agent-orig.exe
        echo done
    fi
else
    echo "No need to backup old ssh-agent - it doesn't exist. Hmmm."
fi

echo -n "Making ssh-agent point symlinkishly to charade ... "
ln -s /usr/bin/charade.exe /usr/bin/ssh-agent.exe
echo done
