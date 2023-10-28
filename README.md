# Simplified File Transfer Protocol

This is an implementation of a simplified version of the file transfer protocol. There are two separate connections made – a control connection used to send FTP commands and responses, and a data connection for sending and receiving data. The server waits on a predefined control port, and the control connection is made by the client. The control connection is kept open until the FTP session is over; however, the data connection can be created and destroyed by the server for each file transfer.

The following set of commands are supported -
- port \<Y\>
- cd \<dir_name\>
- get \<file_name\>
- put \<file_name\>
- quit

The server and the client follow this protocol for file transfer. The file transfer does not happen in one step, as the file may be arbitrarily long. The file is sent in binary mode as a sequence of bytes, block by block. Each block is prefixed with a header containing two fields, a character (‘L’ indicates it is the last block of the file, any other character indicates it is not the last block of the file), and a 16-bit integer (short) indicating the length in bytes of the data in this block sent (not including the header bytes). So a file transfer is over after a block with ‘L’ in the character field is received. Note that this check is sufficient for detecting the end of the file, as TCP ensures that data bytes are received in order. Thus, the sender of the file will read the file block by block, prefix it with the header, and send it.
