#ifndef FPC1020A_h
#define FPC1020A_h

#define ACK_SUCCESS			0x00	// Operation succeeded
#define ACK_FAIL 			0x01	// Operation failed
#define ACK_FULL 			0x04	// Fingerprint database is full
#define ACK_NO_USER 		0x05	// Users do not exist
#define ACK_USER_OCCUPIED 	0x06	// User ID already exists
#define ACK_USER_EXIST		0x07	// Fingerprint already exists
#define ACK_TIMEOUT			0x08	// Accquisition timeout

#define ADD_NEW_USER 		_IOW(0xF5, 1, uint8_t*)
#define FP_SEARCH 			_IOR(0xF5, 2, uint8_t*)
#define USER_COUNT 			_IOR(0xF5, 3, uint8_t*)
#define DELETE_A_USER 		_IOW(0xF5, 4, uint8_t*)
#define DELETE_ALL_USER 	_IO(0xF5, 5)
#define GET_FP_IMAGE 		_IOR(0xF5, 6, uint8_t*)
#define GET_RT_FLAG			_IOR(0xF5, 10, unsigned char*)

#endif
