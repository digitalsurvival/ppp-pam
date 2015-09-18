## Introduction ##

A one-time password (OTP) is a password that is only valid for a single login session or transaction. OTPs avoid a number of shortcomings that are associated with traditional (static) passwords: in contrast to static passwords, they are not vulnerable to replay attacks. This means that, if a potential intruder manages to record an OTP that was already used to log into a service or to conduct a transaction, he will not be able to abuse it since it will be no longer valid. On the downside, OTPs cannot be memorized by human beings. -- Wikipedia

PPP-PAM consists of a pam module and a user utility. PAM module enables (for example) SSH to do a OTP authentication, and the utility manages user keys and prints passcards, like this one:

<pre>
server                            [12]
    A    B    C    D    E    F    G
 1: aj5V rLxi sUb7 W@Un miKM dz:i !z#=
 2: Pme4 gKMx rR#4 :MuA %yjY uvG5 8tfx
 3: AmqM F@p2 Z+2% :8!@ xg#J ELDj FZGr
 4: ky83 3LUh GMp3 ZBz3 Mo9= h6j@ h@6D
 5: UGrU E+9V W8Gm uX:Z DP:L rc66 @vD9
 6: #:yY L::g x6nc Kn2# =o8z B+8C a+Wv
 7: @AfJ heuT igVH Ei?+ o!5q #wWv iz5W
 8: :KEo 6G@= n#gX U6e: mPxz ?u8C kzT:
 9: DkYp rwRg JTA! zJ9a MUY8 p#hk n=EC
10: Nuu! stWi iDDH aSug fRvc aFDB FeER
</pre>

Example usage:

<pre>
user@host $ ssh user@server
Password: user unix password
Passcode 3B [12]: F@p2
user@server $
</pre>

## Installation ##

To enable OTP for ssh sessions:

1. Install the package.
2. Enable its use in `/etc/pam.d`
3. Configure ssh to use PAM.
4. Generate keys for user (and print at least one password card).

So:

#### 1) Package can be installed from the distribution's package repository, or via the classical three step process: ####

 1. <kbd>./configure</kbd>
 2. <kbd>make</kbd>
 3. <kbd>make install</kbd>

#### 2) For exemplary PAM config see pam.d/otp-config. ####

PAM module supports following options:

* enforced   - disallow logon if user does not have `~/.pppauth` directory instead of ignoring OTP.
* bnolock    - disable locking (can cause race conditions).
* secure     - disallow usage of <code>--dontSkip</code> option (dontSkip works bad with locking and can cause some security holes).
* show       - always use passcodes (ignore user options).
* noshow     - never show passcodes (ignore user options).

#### 3) In /etc/ssh/sshd_config you should have the following two lines: ####

<code>ChallengeResponseAuthentication yes</code>

<code>UsePAM yes</code>

#### 4) To generate key, and display first passcard as fast as possible (so you won't loose ability to login in case you're configuring OTP remotely) you can use: ####

<kbd>pppauth --key; pppauth --text --next 1</kbd>

## About PAM (short version) ##

Most application which require password input check the password using PAM. I'll stick to the sshd as an example.

SSH when user logs in tries to authenticate him using it's own method - keys. Then, if this method fails it talks with PAM. PAM to see how to authenticate sshd reads `/etc/pam.d/sshd`.

In default Gentoo installation it will contain following lines:

<pre>
auth       include        system-remote-login
account    include        system-remote-login
password   include        system-remote-login
session    include        system-remote-login
</pre>

This is line-oriented file in which each line tells us what to do. We're interested in "auth" part only, which - includes configuration from system-remote-login, which looks like this:

<pre>auth            include         system-login</pre>

And, as you can see, it just reads configuration from yet another file:

<pre>
auth            required        pam_tally.so onerr=succeed
auth            required        pam_shells.so
auth            required        pam_nologin.so
auth            include         system-auth
</pre>

(account, password, session omitted)

One more file (system-auth) to look into:

<pre>
auth            required        pam_env.so
auth            required        pam_unix.so try_first_pass likeauth nullok
</pre>

PAM when authenticating user will read the lines from top to bottom:

* pam_tally - Reads failures and can do some action according to them.
* pam_shells - Checks if user has a valid shell (listed in `/etc/shells`).
* pam_nologin - Checks if logins were disabled (shows message).
* pam_env - Does something with environment.

And finally:

* pam_unix - Checks password according to `/etc/shadow`.

This is default schema and somewhere there we ought to add our OTP.

Easiest approach just modifies the first file: sshd. After all auth entries we just add our pam_ppp module. The file would look like:

<pre>
auth            include         system-remote-login
# Line added for OTP:
auth            required        pam_ppp.so secure

account    include        system-remote-login
password   include        system-remote-login
session           include        system-remote-login
</pre>

This will ask us for OTP after we are asked for our normal unix password regardless if the unix password was correct or not. This can lead to Denial of Service problem when attacker tries to login enough times to use up all our printed passcards. If we have some other security mechanism (like `sshguard` - which blocks sshd port for people who try the dictionary attacks) it might be perfectly ok.

If not, we can change the line with pam_unix.so module from:

<pre>auth            required        pam_unix.so try_first_pass likeauth nullok</pre>

To:

<pre>auth            requisite       pam_unix.so try_first_pass likeauth nullok</pre>

Which will require correct unix password before asking OTP at all. If we don't like to mess with global pam config files (system-auth etc.) we can move all auth lines to sshd file, change this one line and add pam_ppp line, which results in following configuration:

<pre>
# Include commented out:
#auth       include        system-remote-login
# All auths included in sshd:
auth            required        pam_tally.so onerr=succeed
auth            required        pam_shells.so
auth            required        pam_nologin.so
auth            required        pam_env.so
auth            requisite       pam_unix.so try_first_pass likeauth nullok
# Our line:
auth            required        pam_ppp.so secure

#Rest
account    include        system-remote-login
password   include        system-remote-login
session           include        system-remote-login
</pre>

This is exactly what is written in our `pam.d/otp-login` file. You can place this file in `/etc/pam.d`, and then edit ssh to use it in its auth part:

<pre>
auth       include        otp-login
account    include        system-remote-login
password   include        system-remote-login
session    include        system-remote-login
</pre>

## Generating the key ##

Typing as user the following command:

<kbd>pppauth -k </kbd>

This will create the `~/.pppauth` directory, generate a key, and enable OTP for the user.

After generating key it is important to remember to print yourself a card of passwords:

<kbd>pppauth -t -c 1</kbd>

Copy/paste/print (or use pppauth -t -c 1 | lp) Or...

<kbd>pppauth -h -c 1</kbd>

And use a browser to print a card.

Or:

$ pppauth -l -c 1 > file.latex
$ pdflatex file.latex

And then print file.pdf containing 6 cards.