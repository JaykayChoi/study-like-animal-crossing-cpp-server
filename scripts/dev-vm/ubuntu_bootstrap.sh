#!/bin/bash

###############################################################################
#
# Bootstrap Dev VM.
#
###############################################################################

# Check priviledge
if [ "$(id -u)" != "0" ]; then
	echo "You must have 'sudo' priviledge to execute this script."
	exit 1
fi

# Update and upgrade.
apt-get -y update
apt-get -y upgrade

# Install and setup vim.
apt-get -y install vim
bash -c 'echo "set tabstop=2 softtabstop=2 expandtab shiftwidth=2 smarttab" >> /etc/vim/vimrc'

# Install curl.
apt-get -y install curl

# Install jq.
apt-get -y install jq

# Install git.
apt-add-repository -y ppa:git-core/ppa
apt-get -y update
apt-get -y install git

# Install Node.js.
curl -sL https://deb.nodesource.com/setup_12.x | sudo -E bash -
apt-get install -y nodejs

# Install Yarn.
curl -sS https://dl.yarnpkg.com/debian/pubkey.gpg | sudo apt-key add -
echo "deb https://dl.yarnpkg.com/debian/ stable main" | sudo tee /etc/apt/sources.list.d/yarn.list
apt-get -y update
apt-get -y install yarn

# Install Visual Studio Code.
curl https://packages.microsoft.com/keys/microsoft.asc | gpg --dearmor > microsoft.gpg
mv microsoft.gpg /etc/apt/trusted.gpg.d/microsoft.gpg
sh -c 'echo "deb [arch=amd64] https://packages.microsoft.com/repos/vscode stable main" > /etc/apt/sources.list.d/vscode.list'
apt-get -y update
apt-get -y install code

# Install ssh.
apt-get -y install openssh-server

# Install docker.
curl -fsSL https://download.docker.com/linux/ubuntu/gpg | apt-key add -
add-apt-repository "deb [arch=amd64] https://download.docker.com/linux/ubuntu $(lsb_release -cs) stable"
apt-get update
apt-get install -y docker-ce

# Install docker-compose
curl -L https://github.com/docker/compose/releases/download/1.19.0/docker-compose-`uname -s`-`uname -m` -o /usr/local/bin/docker-compose
chmod +x /usr/local/bin/docker-compose

# Install mysql client.
apt-get -y install mysql-client

# Install global npm packages.
npm install -g \
  pm2 \
  eslint \
  eslint-plugin-import \
  eslint-plugin-node \
  eslint-plugin-promise \
  eslint-plugin-standard \
  eslint-config-standard \

