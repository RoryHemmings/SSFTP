# SSFTP

SSFTP (Secure Simple File Transfer Protocol) is an updated file transfer protocol based on the Simple File Transfer Protocol, but with more modern features and using well defined binary packet structure instead of strings. This implementation is an example of how the protocol could be implemented.

General Notes:
	* asynchronus server handles multiple file transfers at the same time 

Commands Structure
There are two types of commands, sftp commands which are refered to as commands and local commands which are refered to as L_COMMANDS. The local are the commands that are issued by the user via stdin.
  * Some commands are async
    - this means that they are multi stage and that 
      they break up their load into mutliple stages

  - command type: 1 byte
  - header: (different length for each command)
  - data: (different length for each command)
  - null termination: 1 byte

Response Structure
  - success/failure: 1 byte (either + or -)
  - data (if operation succeeds): variable length
  - error code (if operation fails): 1 bytes (8 bit unsigned int)
  - null termination: 1 byte

Commands
  - USER (0000 0001) (0x01)
    * header
      + username-length (16 bit unsigned int)
      + password-length (16 bit unsigned int)
    * data
      + username (character string) NO NULL TERMINATION BYTE (max length 512)
      + password (character string) NO NULL TERMINATION BYTE (max length 512)
    * null termination (1 byte char) 
    * response
      + standard

  - PRWD (0000 0010) (0x02)
    * header
      + none
    * data
      + none
    * null termination (1 byte char)
    * response
      + standard

  - LIST (0000 0011) (0x03) ASYNC
    * header
      + none
    * data
      + none
    * null termination (1 byte char)
    * response: (may contain multiple packets)
      + Primary Packet
        - 1 byte status
        - total-packets (32 bit unsigned int)
        - null termination
      + Secondary packets
        - 1 byte status
        - data - max: 1022 bytes
        - null termination

  - CDIR (0000 0100) (0x04)
    * header
      + none
    * data
      + directory (character string) NO NULL TERMINATION
    * null termination (1 byte char)
    * response
      + 1 byte status
      + new path (character string) NO NULL TERMINATION
      + 1 byte null termination

  - GRAB (0000 0111) (0x07) ASYNC
    * header
      + none
    * data
      + file name (or path) (character string) NO NULL TERMINATION
    * null termination (1 byte char)
    * response
      + Primary Packet
        - 1 byte status
        - total-packets (32 bit unsigned int)
        - file-name (character string)
        - null termination
      + Secondary Packets
        - 1 byte status
        - length of data: (1 bit unsigned int)
        - data - max: 1022 bytes
        - null termination


Responses 
  - SUCCESS (0000 0001) (0x01)
  - ERROR (0000 0010) (0x02)

Errors:
  - All errors are 8 bit integers
  - Remote/Protocol errors are positive
  - Local errors are negative

