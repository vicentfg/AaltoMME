MME - Mobility management entity
===================================
This application is a C implementation of the EPS Mobility management entity. It has the S11, S1-MME interfaces and the S6a emulated with an internal HSS.


Dependencies
------------
- pkg-config
- check (http://sourceforge.net/projects/check/) (optional)
   - flex (optional)
   - bison (optional)
- libconfig9 (http://www.hyperrealm.com/libconfig/libconfig.html)
- glib
- libsctp
- MariaDB server and client (mariadb-server libmariadbclient-dev)


3rd Party Sources included on the project
-----------------------------------------
- HMAC library (https://github.com/ouah/hmac)
- AES & milenage (http://svn0.us-east.freebsd.org/base/vendor/hostapd/0.5.8/contrib/hostapd/)
- HASH Table (http://uthash.sourceforge.net)


Build
-----
Two different build systems are provided, CMake (preferred) and autotools.

### CMake
Use the usual cmake workflow. To build a release version (out-of-tree):

```
$ mkdir Release && cd Release
$ cmake -DCMAKE_BUILD_TYPE=Release ..
$ make
```

A packet can be also created using CPack

```
$ cpack -G DEB
```

### Autotools

```
$ libtoolize
$ autoreconf -i
$ ./configure
$ make
```

Setup HSS database
------------------
1. Install MariaDB or MySQL on your machine (if you haven't already done so), MariaDB repository configuration tool: https://downloads.mariadb.org/mariadb/repositories/ . Don't forget the client development packet, libmariadbclient-dev on Debian.

2. Run the script that creates the database and the tables.
```
$ mysql -u admin -p < mme/S6a/scripts/hss_lte_db.sql
```

3. Run the script that creates demo user
```
$ mysql -u admin -p < mme/S6a/scripts/userdata.sql
```

4. The user and password for interacting with mysql are hss:hss


Execution
---------
There are some enviromental variables to configure the MME:
- MME_LOGLEVEL: sets the inial log level of the MME. Range 1-7, equivalent to syslog.
- MME_CONFIG: path to mme.cfg file. Check the example in the repository.


MME CLI
-------
The CLI can be accesses on the TCP port 5000 using netcat. It provides basic functionality as:
- change log level
- view S1 associations


Credit
------
This application was initially developed as a Final Project by
    Vicent Ferrer Guasch (vicent.ferrerguasch@aalto.fi)
Under the supervision of,
    Jukka Manner (jukka.manner@aalto.fi)
    Jose Costa-Requena (jose.costa@aalto.fi)
in AALTO University and partially funded by EIT ICT labs.


Unit testing
------------
This project was using check framework at the begining, but now it includes some tests using glib.

- Check Test framework: http://check.sourceforge.net/
```
$ make check
```


Jenkins Job Configuration (Old)
-------------------------------

### Shell 1 (Build Code):
```libtoolize -i
autoreconf -i
./configure --enable-debug --enable-gcov
make
```

### Shell 2 (Run Unit Test Framework):
```
bash -c "make check ; exit 0 "
```

### Shell 3 (Run Static Metrics):
```
cppcheck . --xml 2> cppcheck-result.xml
find .  \( \( \( -name *.c \) -o \( -name *.h \) \) -a \( ! -name xml* \) \) -print | cccc --lang="c" -
sloccount --duplicates --wide --details . | grep ansic > sloccount.sc
bash -c "/usr/local/bin/pmd/bin/run.sh cpd --minimum-tokens 50 --files . --language c --format xml 1>cpd.xml ; exit 0"
```

### Shell 4 (Build Documentation):
```
doxygen mme.doxyfile
```

### Shell5 (Measure code coverage):
```
cd TestProgram
gcov *.o
cd ..
gcovr -x > coverage.xml
```

### Publish results:
- CPD:			**/cpd.xml
- CCCC:			.cccc/cccc.xml
- Covertura:		**/coverage.xml
- CppCheck:		**/cppcheck-result.xml
- Doxygen path:	mme.doxyfile
- SLOCcount:		**/sloccount.sc
- xUnit:			**/check_tests.xml


Old
---
* Debugging

To build the project with debugging symbols, use the configure flag --enable-debug

* Code Coverage
To enable code coverage use the configure flags --enable-debug --enable-gcov
The code coverage is obtained with gcov and gcovr. (gcovr allows the integration on Jenkins using coverage plugin)

To install gcovr:
```
$ wget https://bitbucket.org/pypa/setuptools/raw/bootstrap/ez_setup.py -O - | sudo python
$ svn co https://software.sandia.gov/svn/public/fast/gcovr/trunk/ gcovr
$ cd gcovr
$ sudo python setup.py install
```


Test bed execution
------------------

### nwLteSaeGw
- NAT is required to have an IP pool address available to assign to the UEs. The nwLteSaeGw is not able to asnwer ARP requests, to solve this issue the arp table spoofing is considered as a temporal solution:
```
$ arpspoof -i <iface> -t <NAT route> <UE IP>
```

- Other requirements detailed on nwLteSaeGw README.

### MME
    - The MME configuration is obtained from the mme.cfg file
    - (Autotools) Before executing the MME don't forget to set the required library path, i.e. :
        $ export LD_LIBRARY_PATH=~/MME/libgtp/.libs/:~/MME/S1AP/src/.libs/:~/MME/NAS/src/.libs/
    - The MME currently uses the same NIC for S1 and S11.

### Firewall
    Disable the firewall if there are connection problems like
    "Destination Unreachable" - "Host Administratively prohibited"
