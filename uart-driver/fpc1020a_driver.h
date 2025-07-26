#ifndef FPC1020A_DRIVER_h
#define FPC1020A_DRIVER_h

// Defining FPC1020A module's commands and flags
#define ACK_SUCCESS			0x00	// Operation succeeded
#define ACK_FAIL 			0x01	// Operation failed
#define ACK_FULL 			0x04	// Fingerprint database is full
#define ACK_NO_USER 		0x05	// Users do not exist
#define ACK_USER_OCCUPIED 	0x06	// User ID already exists
#define ACK_USER_EXIST		0x07	// Fingerprint already exists
#define ACK_TIMEOUT			0x08	// Accquisition timeout

#define CMD_ENROLL1			0x01	// Add new fingerprint 1st time
#define CMD_ENROLL2			0x02	// Add new fingerprint 2nd time
#define CMD_ENROLL3 		0x03	// Add new fingerprint 3rd time
#define CMD_DELETE_USER		0x04	// Delete assigned user
#define CMD_DELETE_ALL 		0x05	// Delete all users
#define CMD_USER_COUNT 		0x09	// Get the number of users
#define CMD_IDENTIFY 		0x0B	// Fingerprint matching 1:1
#define CMD_SEARCH 			0x0C	// Fingerprint matching 1:N
#define CMD_GET_USER_ID 	0x2B	// Get user ID and user permission
#define CMD_GET_IMAGE 		0x24	// Get user's fingerprint image

#define DATA_START 			0xF5	// Data start byte
#define DATA_END 			0xF5	// Data end byte

#define IRG_NO 11

// Defining ioctl commands to be called
#define ADD_NEW_USER 		_IOW(0xF5, 1, uint8_t*)
#define FP_SEARCH 			_IOR(0xF5, 2, uint8_t*)
#define USER_COUNT 			_IOR(0xF5, 3, uint8_t*)
#define DELETE_A_USER 		_IOW(0xF5, 4, uint8_t*)
#define DELETE_ALL_USER 	_IO(0xF5, 5)
#define GET_FP_IMAGE 		_IOR(0xF5, 6, uint8_t*)
#define GET_RT_FLAG			_IOR(0xF5, 10, unsigned char*)

#endif
