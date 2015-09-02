_**Please note that these instructions differ from the initial instructions published in the GRC newsgroups.  The most recent instructions will appear here, so you should always check for the latest version of this page before attempting to build the code.**_

# Mac OSX #

The following should work on **Tiger** and **Leopard** for both **Intel** and **Power PC** Macs:

  1. Download version 0.2 of the source code from [here](http://ppp-pam.googlecode.com/files/ppp-pam-0.2.tar.gz) and save to your disk.
  1. Open a terminal window and extract the source files.
```
tar -xvzf ppp-pam-0.2.tar.gz
cd ppp-pam
```
  1. Build the code
```
cd build
../configure
make
```
  1. Test to confirm it built correctly
```
make test
```
  1. Install the `pppauth` utility and PAM module in the appropriate folders. (You will need to enter your administrator password to run the following command).
```
sudo make install
```
  1. Enable PPP authentication for ssh connections.
```
sudo chmod +w /etc/pam.d/sshd
sudo open -a TextEdit /etc/pam.d/sshd
```
> > Enter the following line just above the line with `pam_securityserver.so`
```
  auth       required       pam_ppp.so
```
  1. Save the file and close TextEdit.
  1. Create a PPP sequence key for your user account.
```
pppauth --key
```
  1. Print some passcards.
```
pppauth --html --next 3
```
  1. Try logging in to test it.
```
ssh localhost
```

# Linux #

It's still not working reliably enough on all distributions of linux.  I need to do more portability work, but it is getting closer!

  1. Make sure you have the appropriate packages installed.  On a fresh Ubuntu distribution, I had to install the following packages:
    * subversion
    * make
    * gcc
    * g++
    * libc6-dev
    * uuid-dev
    * libpam0g-dev
    * openssh-server
  1. Follow steps 1-5 of the Mac OSX installation above.
  1. Enable PPP authentication for ssh connections.  (Feel free to use your preferred editor rather than vi).  The specifics here may vary depending on your linux distribution.  If you find that they deviate significantly, please post a comment here.
```
sudo vi /etc/pam.d/ssh
```
> > Enter the following line just below `@include common-auth`
```
  auth       required       pam_ppp.so
```
  1. Close and save the file.
  1. Make sure you have the following settings in `/etc/ssh/sshd_config`:
```
ChallengeResponseAuthentication yes
UsePAM yes
```
  1. Create a PPP sequence key for your user account.
```
pppauth --key
```
  1. Generate a passcard.  Print or save it -- you'll need it to log in over SSH.
```
pppauth --text --next 1
```
  1. Try logging in to test it.
```
ssh localhost
```

## Instructions for Specific Linux Distributions ##

#### SuSe 64-bit Linux ####
You may need to install the following packages:
  * pam-devel-0.99.8.1-15.i586.rpm
  * libuuid-devel-1.40.2-20.i586.rpm
Also, `pam_ppp.so` must go in `/lib64/security`.  At the moment, `sudo make install` attepts to put it in `lib/security` so you will need to copy it manually:
```
sudo cp pam_ppp.so /lib64/security/pam_ppp.so
```