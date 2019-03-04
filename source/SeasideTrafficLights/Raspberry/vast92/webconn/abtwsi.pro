# This is the properties file for using the VA Smalltalk Web Connection
# Servlet interface. This file should be in your JAVA "user.dir" directory.


# ABTWSI_BASENAME contains a directory where VA Smalltalk Web Connection
# configuration files are found. The configuration file name should be appended at 
# the end of the entry. The following shows a path where the abtwsi.* files have
# been renamed to mywsi.*
# Use forward slashes '/' as shown below when specifying the path information.
# Java treats backward slashes '\' as escape characters.
#        ABTWSI_BASENAME=d:/vast/wsifiles/mywsi       
ABTWSI_BASENAME=c:/vast/abtwsi

# SOCKET_TIMEOUT is the time in milliseconds that a socket (from the Java 
# servlet) waits for a reply from the Smalltalk Web Connection part. 
# A value of 0 (zero) indicates no timeout. Default timeout is 15000.
SOCKET_TIMEOUT=15000
