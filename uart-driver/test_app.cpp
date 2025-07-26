#include <iostream>
#include <vector>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <opencv2/highgui.hpp>
#include "fpc1020a.h"

using namespace std;
int main()
{
	// g++ test_app.cpp -o test_app `pkg-config --cflags --libs opencv4`
	int fd;
	unsigned int u_id = 0;
	unsigned char mode = 0;
	unsigned char rtflag = 0;
	unsigned int u_count = 0;
	unsigned char deleteflag = 0;
	uint8_t img_matrix[80][40];
	
	printf("Opening Driver\n");
	fd = open("/dev/fpc1020a_dev", O_RDWR);
	if(fd < 0){
		cout << "Cannot open device file" << endl;
		return 0;
	}
	
	cout << "Fingerprint module FPC1020A Test Application" << endl;
	cout << "Choose one of  the options below\n" << endl;
	
	while(1){
		mode = 0;
		u_id = 0;
		rtflag = 0;
		
		cout << "==================== Menu ====================" << endl;
		cout << "Add a new user-------------------------------1" << endl;
		cout << "Fingerprint matching-------------------------2" << endl;
		cout << "Get number of user---------------------------3" << endl;
		cout << "Delete assigned user-------------------------4" << endl;
		cout << "Delete all user------------------------------5" << endl;
		cout << "Get fingerprint image------------------------6" << endl;
		cout << "Exit program---------------------------------7" << endl;
		cout << "==================== End =====================" << endl;
		
		cin >> mode;
		mode = mode - 0x30;
		switch(mode){
			case 1:
				cout << "Please input the new user ID (0 ~ 99):  ";
				while(!(cin >> u_id));
				usleep(100000);
				cout << "Please put your fingerprint on the sensor" << endl;
				cout << "Wait for the sensor to retrieve data" << endl;
				
				ioctl(fd, ADD_NEW_USER, (unsigned int*) &u_id);
				ioctl(fd, GET_RT_FLAG, (unsigned char*) &rtflag);
				
				if(rtflag == ACK_SUCCESS) {
					cout << "Adding new user succeeded!" << endl;
				} else if(rtflag == ACK_FAIL) {
					cout << "Adding new user failed!" << endl;
				} else if(rtflag == ACK_USER_OCCUPIED) {
					cout << "Failed, this user ID has already existed!" << endl;
				} else if(rtflag == ACK_USER_EXIST) {
					cout << "Failed, this fingerprint has already existed!" << endl;
				}
				cout << endl;
				cout << endl;
				usleep(2000000);
				break;
				
			case 2:
				cout << "Match fingerprint, please put your finger on the sensor." << endl;
				ioctl(fd, FP_SEARCH, (unsigned int*) &u_id);
				ioctl(fd, GET_RT_FLAG, (unsigned char*) &rtflag);
				
				if(rtflag == ACK_SUCCESS) {
					cout << "Match success. Found a fingerprint with user ID of " << unsigned(u_id) << endl;
				} else {
					cout << "Failed, please try again." << endl;
				}
				cout << endl;
				cout << endl;
				usleep(1000000);
				break;
				
			case 3:
				cout << "Wait for the module to count users" << endl;
				ioctl(fd, USER_COUNT, (unsigned int*) &u_count);
				ioctl(fd, GET_RT_FLAG, (unsigned char*) &rtflag);
				
				if(rtflag == ACK_SUCCESS) {
					cout << "The number of user is " << unsigned(u_count) << endl;
				} else {
					cout << "Failed, please try again." << endl;
				}
				cout << endl;
				cout << endl;
				usleep(1000000);
				break;
			
			case 4:
				cout << "Please input the user ID(0 ~ 99) to delete from the database.  ";
				while(!(cin >> u_id));
				usleep(100000);
				
				ioctl(fd, DELETE_A_USER, (unsigned int*) &u_id);
				ioctl(fd, GET_RT_FLAG, (unsigned char*) &rtflag);
				
				if(rtflag == ACK_SUCCESS) {
					cout << "Delete the user success!"<< endl;
				} else {
					cout << "Delete the user fail!" << endl;
				}
				cout << endl;
				cout << endl;
				usleep(1000000);
				break;
				
			case 5:
				deleteflag = 0;
				cout << "Delete all users? Y/N ?" << endl;
				
				// Wait for response for 4 seconds
				for(unsigned char i = 200; i > 0; i--) {
					usleep(20000);
					while(!(cin >> deleteflag)) {
						break;
					}
					break;
				}
				
				if(deleteflag == 'Y' || deleteflag == 'y') {
					ioctl(fd, DELETE_ALL_USER);
					ioctl(fd, GET_RT_FLAG, (unsigned char*) &rtflag);
					if(rtflag == ACK_SUCCESS) {
						cout << "Delete all users success!" << endl;
					} else {
						cout << "Delete all users fail!" << endl;
					}
				} else if(deleteflag == 'N' || deleteflag == 'n') {
					cout << "Canceled" << endl;
				} else {
					cout << "Option not available. Deletion failed!" << endl;
				}
				cout << endl;
				cout << endl;
				usleep(1000000);
				break;
				
			case 6:
				cout << "Please put your fingerprint on the sensor to retrieve image data" << endl;
				ioctl(fd, GET_FP_IMAGE, (unsigned int*) &img_matrix);
				ioctl(fd, GET_RT_FLAG, (unsigned char*) &rtflag);
				if(rtflag == ACK_SUCCESS) {
					cout << "Retrieved image data!" << endl;
					cv::Mat imgGray(80, 40, CV_8UC1, img_matrix);
					cv::namedWindow("Test", cv::WINDOW_NORMAL);
					cv::resizeWindow("Test", 200, 400);
					cv::imshow("Test", imgGray);
					cv::waitKey(0);
				} else {
					cout << "Retrieve fingerprint image fail!" << endl;
				}
				cout << endl;
				cout << endl;
				usleep(1000000);
				break;
				
			case 7:
				close(fd);
				exit(0);
				break;
				
			default:
				break;
		}
	}
}
