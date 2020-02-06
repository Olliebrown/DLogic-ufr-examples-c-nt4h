/*
 ============================================================================
 Project Name: project_name
 Name        : file_name.c
 Author      : d-logic (http://www.d-logic.net/nfc-rfid-reader-sdk/)
 Version     :
 Copyright   : 2017.
 Description : Project in C (Language standard: c99)
 Dependencies: uFR firmware - min. version x.y.z {define in ini.h}
               uFRCoder library - min. version x.y.z {define in ini.h}
 ============================================================================
 */

/* includes:
 * stdio.h & stdlib.h are included by default (for printf and LARGE_INTEGER.QuadPart (long long) use %lld or %016llx).
 * inttypes.h, stdbool.h & string.h included for various type support and utilities.
 * conio.h is included for windows(dos) console input functions.
 * windows.h is needed for various timer functions (GetTickCount(), QueryPerformanceFrequency(), QueryPerformanceCounter())
 */
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdbool.h>
#include <string.h>
#if __WIN32 || __WIN64
#	include <conio.h>
#	include <windows.h>
#elif linux || __linux__ || __APPLE__
//#	define __USE_MISC
#	include <unistd.h>
#	include <termios.h>
//#	undef __USE_MISC
#	include "conio_gnu.h"
#else
#	error "Unknown build platform."
#endif
#include <uFCoder.h>
#include "ini.h"
#include "uFR.h"
#include "utils.h"
//------------------------------------------------------------------------------
void usage(void);
void menu(char key);
UFR_STATUS NewCardInField(uint8_t sak, uint8_t *uid, uint8_t uid_size);
void get_file_setting(void);
void set_file_setting(void);
void get_uid(void);
void set_rid(void);
void change_key(void);
void linear_read(void);
void linear_write(void);
void sdm_read(void);
void sdm_write(void);
void read_sdm_counter(void);
void store_key(void);

//------------------------------------------------------------------------------
int main(void)
{
	char key;
	bool card_in_field = false;
	uint8_t old_sak = 0, old_uid_size = 0, old_uid[10];
	uint8_t sak, uid_size, uid[10];
	UFR_STATUS status;

	printf(" --------------------------------------------------\n");
	printf("     Please wait while opening uFR NFC reader.\n");
	printf(" --------------------------------------------------\n");

	char mode;
	printf("Select reader opening mode:\n");
	printf(" (1) - Simple Reader Open\n");
	printf(" (2) - Advanced Reader Open\n");
	mode = _getch();
	fflush(stdin);
	if (mode == '1')
	{
		status = ReaderOpen();
	}
	else if (mode == '2')
	{
	   uint32_t reader_type = 1;
	   char port_name[1024] = "";
	   uint32_t port_interface = 2;
	   char open_args[1024] = "";
	   char str_interface[2] = "";
	   char port_exist[2] = "";

	   printf("Enter reader type:\n");
	   scanf("%d", &reader_type);
	   fflush(stdin);

	   printf("Does exist port name (y or n)\n");
	   scanf("%s", port_exist);
	   fflush(stdin);
	   if(port_exist[0] == 'y' || port_exist[0] == 'Y')
	   {
		   printf("Enter port name:\n");
		   scanf("%s", port_name);
		   fflush(stdin);
	   }

	   printf("Enter port interface:\n");
	   scanf("%s", str_interface);
	   if (str_interface[0] == 'U')
		   port_interface = 85;
	   else if (str_interface[0] == 'T')
		   port_interface = 84;
	   else
	   	   port_interface = atoi(str_interface);

	   fflush(stdin);

	   printf("Enter additional argument:\n");
	   scanf("%s", open_args);
	   fflush(stdin);

	   status = ReaderOpenEx(reader_type, port_name, port_interface, open_args);
	}
	else
	{
		printf("Invalid input. Press any key to quit the application...");
		getchar();
		return EXIT_FAILURE;
	}

	if (status != UFR_OK)
	{
		printf("Error while opening device 1, status is: 0x%08X\n", status);
		getchar();
		return EXIT_FAILURE;
	}

	if (!CheckDependencies())
	{
		ReaderClose();
		getchar();
		return EXIT_FAILURE;
	}

	printf(" --------------------------------------------------\n");
	printf("        uFR NFC reader successfully opened.\n");
	printf(" --------------------------------------------------\n");

	usage();

#if linux || __linux__ || __APPLE__
	_initTermios(0);
#endif
	do
	{
		while (!_kbhit())
		{
			status = GetCardIdEx(&sak, uid, &uid_size);
			switch (status)
			{
				case UFR_OK:
					if (card_in_field)
					{
						if (old_sak != sak || old_uid_size != uid_size || memcmp(old_uid, uid, uid_size))
						{
							old_sak = sak;
							old_uid_size = uid_size;
							memcpy(old_uid, uid, uid_size);
							NewCardInField(sak, uid, uid_size);
						}
					}
					else
					{
						old_sak = sak;
						old_uid_size = uid_size;
						memcpy(old_uid, uid, uid_size);
						NewCardInField(sak, uid, uid_size);
						card_in_field = true;
					}
					break;
				case UFR_NO_CARD:
					card_in_field = false;
					status = UFR_OK;
					break;
				default:
					ReaderClose();
					printf(" Fatal error while trying to read card, status is: 0x%08X\n", status);
					getchar();
#if linux || __linux__ || __APPLE__
					_resetTermios();
					tcflush(0, TCIFLUSH); // Clear stdin to prevent characters appearing on prompt
#endif
					return EXIT_FAILURE;
			}
#if __WIN32 || __WIN64
			Sleep(300);
#else // if linux || __linux__ || __APPLE__
			usleep(300000);
#endif
		}

		key = _getch();
		menu(key);
	}
	while (key != '\x1b');

	ReaderClose();
#if linux || __linux__ || __APPLE__
	_resetTermios();
	tcflush(0, TCIFLUSH); // Clear stdin to prevent characters appearing on prompt
#endif
	return EXIT_SUCCESS;
}
//------------------------------------------------------------------------------
void menu(char key)
{

	switch (key)
	{
		case '1':
			get_file_setting();
			break;

		case '2':
			set_file_setting();
			break;

		case '3':
			get_uid();
			break;

		case '4':
			set_rid();
			break;

		case '5':
			change_key();
			break;

		case '6':
			linear_read();
			break;

		case '7':
			linear_write();
			break;

		case '8':
			sdm_read();
			break;

		case '9':
			sdm_write();
			break;

		case 'a':
		case 'A':
			read_sdm_counter();
			break;

		case 'b':
		case 'B':
			store_key();
			break;

		case '\x1b':
			break;

		default:
			usage();
			break;
	}
}
//------------------------------------------------------------------------------
void usage(void)
{
		printf(" +------------------------------------------------+\n"
			   " |                   NT4H example                 |\n"
			   " |                 version "APP_VERSION"                    |\n"
			   " +------------------------------------------------+\n"
			   "                              For exit, hit escape.\n");
		printf(" --------------------------------------------------\n");
		printf("  (1) - get file setting\n"
			   "  (2) - set file setting\n"
			   "  (3) - get UID (NTAG424 only)\n"
			   "  (4) - set random ID (NTAG424 only)\n"
			   "  (5) - change AES key\n"
			   "  (6) - linear_read\n"
			   "  (7) - linear write\n"
			   "  (8) - secure dynamic message read\n"
			   "  (9) - secure dynamic message write\n"
			   "  (a) - get sdm reading counter\n"
			   "  (b) - store AES key into reader\n");
}
//------------------------------------------------------------------------------
UFR_STATUS NewCardInField(uint8_t sak, uint8_t *uid, uint8_t uid_size)
{
	UFR_STATUS status;
	uint8_t dl_card_type;

	status = GetDlogicCardType(&dl_card_type);
	if (status != UFR_OK)
		return status;

	printf(" \a-------------------------------------------------------------------\n");
	printf(" Card type: %s, sak = 0x%02X, uid[%d] = ", GetDlTypeName(dl_card_type), sak, uid_size);
	print_hex_ln(uid, uid_size, ":");
	printf(" -------------------------------------------------------------------\n");

	return UFR_OK;
}
//------------------------------------------------------------------------------
bool enter_aes_key(uint8_t *aes_key)
{
	char str[100];
	size_t str_len;

	scanf("%[^\n]%*c", str);
	str_len = hex2bin(aes_key, str);
	if(str_len != 16)
	{
		printf("\nYou need to enter 16 hexadecimal numbers with or without spaces or with : as delimiter\n");
		scanf("%[^\n]%*c", str);
		str_len = hex2bin(aes_key, str);
		if(str_len != 16)
			return false;
	}

	return true;
}
//------------------------------------------------------------------------------
bool enter_linear_data(uint8_t *linear_data, uint16_t *linear_len)
{
	char str[257];
	size_t str_len;
	char key;

	*linear_len = 0;

	printf(" (1) - ASCI\n"
		   " (2) - HEX\n");

	while (!_kbhit())
		;
	key = _getch();

	if(key == '1')
	{
		printf("Enter ASCI text\n");
		scanf("%[^\n]%*c", str);
		str_len = strlen(str);
		*linear_len = str_len;
		memcpy(linear_data, str, *linear_len);
		return true;
	}
	else if(key == '2')
	{
		printf("Enter hexadecimal bytes\n");
		scanf("%[^\n]%*c", str);
		str_len = hex2bin(linear_data, str);
		*linear_len = str_len;
		return true;
	}
	else
		return false;
}
//-----------------------------------------------------------------------------

void enter_ascii_data(uint8_t *data, uint8_t *length)
{
	char str[257];
	size_t str_len;


	*length = 0;

	printf("Enter ASCI text\n");
	scanf("%[^\n]%*c", str);
	str_len = strlen(str);
	*length = str_len;
	memcpy(data, str, *length);
}


//------------------------------------------------------------------------------
void get_file_setting(void)
{
	printf(" -------------------------------------------------------------------\n");
	printf("                           Get file setting                         \n");
	printf(" -------------------------------------------------------------------\n");

	UFR_STATUS status;
	uint8_t file_no, file_type, communication_mode, sdm_enable;
	uint8_t read_key_no, write_key_no, read_write_key_no, change_key_no;
	uint8_t uid_enable, read_ctr_enable, read_ctr_limit_enable, enc_file_data_enable;
	uint8_t meta_data_key_no, file_data_read_key_no, read_ctr_key_no;
	uint32_t file_size = 0, uid_offset = 0, read_ctr_offset = 0, picc_data_offset = 0;
	uint32_t mac_input_offset = 0, enc_offset = 0, enc_length = 0, mac_offset = 0, read_ctr_limit = 0;
	int file_no_int;

	printf("\nEnter file number (1 - 2 for NTAG413) (1 - 3 for NTAG424)\n");
	scanf("%d%*c", &file_no_int);
	file_no = file_no_int;

	status = nt4h_get_file_settings(file_no, &file_type, &communication_mode, &sdm_enable, &file_size,
									&read_key_no, &write_key_no, &read_write_key_no, &change_key_no,
									&uid_enable, &read_ctr_enable, &read_ctr_limit_enable, &enc_file_data_enable,
									&meta_data_key_no, &file_data_read_key_no, &read_ctr_key_no,
									&uid_offset, &read_ctr_offset, &picc_data_offset,
									&mac_input_offset, &enc_offset, &enc_length, &mac_offset, &read_ctr_limit);

	if(status)
	{
		printf("\nGet file setting failed\n");
		printf("Error code = 0x%08X\n", status);
	}
	else
	{
		printf("\nGet file setting successful\n");

		printf("File type: Standard data file\n");

		switch(communication_mode)
		{
		case 0:
			printf("Communication mode: plain\n");
			break;
		case 1:
			printf("Communication mode: macked\n");
			break;
		case 3:
			printf("Communication mode: enciphered\n");
			break;
		}

		if(sdm_enable)
			printf("Secure dynamic messaging: enabled\n");
		else
			printf("Secure dynamic messaging: disabled\n");

		printf("File access rights (0x0E - free access, 0x0F - no access)\n");
		printf("Read key: 0x%02X\n", read_key_no);
		printf("Write key: 0x%02X\n", write_key_no);
		printf("ReadWrite key: 0x%02X\n", read_write_key_no);
		printf("Change key: 0x%02X\n", change_key_no);

		printf("File size: %d\n", file_size);

		if(sdm_enable)
		{
			printf("Secure dynamic message options\n");

			if(uid_enable)
				printf("UID mirroring: enabled\n");
			else
				printf("UID mirroring: disabled\n");

			if(read_ctr_enable)
				printf("Read counter: enabled\n");
			else
				printf("Read counter: disabled\n");

			if(read_ctr_limit_enable)
				printf("Read counter limit: %d\n", read_ctr_limit);
			else
				printf("Read counter limit: disabled\n");

			if(enc_file_data_enable)
				printf("Encrypted part of file data: enabled\n");
			else
				printf("Encrypted part of file data: disabled\n");

			printf("SDM access rights (0x0E free/plain, 0x0F no access/no data\n");
			printf("SDM meta read: 0x%02X\n", meta_data_key_no);
			printf("SDM file key: 0x%02X\n", file_data_read_key_no);
			printf("SDM reading counter read key: 0x%02X\n", read_ctr_key_no);

			if(meta_data_key_no == 0x0E)
			{
				//plain PICC data mirroring
				if(uid_enable)
					printf("UID offset: %d\n", uid_offset);
				if(read_ctr_enable)
					printf("Read counter offset: %d\n", read_ctr_offset);
			}
			else if(meta_data_key_no == 0x0F)
				printf("No PICC data mirroring\n");
			else
			{
				//enciphered PICC data
				printf("PICC data offset: %d\n", picc_data_offset);
			}

			if(file_data_read_key_no != 0x0F)
			{
				//MAC exist
				printf("MAC input data offset: %d\n", mac_input_offset);
				if(enc_file_data_enable)
				{
					//Part of file data exist
					printf("Encrypted data offset: %d\n", enc_offset);
					printf("Encrypted data length: %d\n", enc_length);
				}
				printf("MAC offset: %d\n", mac_offset);
			}
		}
	}
}
//------------------------------------------------------------------------------
void set_file_setting(void)
{
	printf(" -------------------------------------------------------------------\n");
	printf("                           Set file setting                         \n");
	printf(" -------------------------------------------------------------------\n");

	UFR_STATUS status;
	char key;
	uint8_t smd_data_file;
	uint8_t aes_key_ext[16];
	uint8_t file_no, key_no, curr_communication_mode, new_communication_mode;
	uint8_t read_key_no, write_key_no, read_write_key_no, change_key_no;
	uint8_t internal_key, aes_internal_key_no;

	int file_no_int, key_no_int;
	int read_key_no_int, write_key_no_int, read_write_key_no_int, change_key_no_int;
	int aes_internal_key_no_int;

	printf(" Select file type\n");
	printf(" (1) - Standard data file\n");
	printf(" (2) - Secure messaging data file\n");
	key = _getch();
	if(key == '2')
		smd_data_file = 1;
	else
		smd_data_file = 0;

	printf("\nEnter file number (1 - 2 for NTAG413) (1 - 3 for NTAG424)\n");
	scanf("%d%*c", &file_no_int);
	file_no = file_no_int;

	printf("\nEnter change key number (0 - 2 for NTAG413) (0 - 4 for NTAG424)\n");
	scanf("%d%*c", &key_no_int);
	key_no = key_no_int;

	curr_communication_mode = 3;

	printf("File access rights (14 - free access, 15 - no access)\n");
	printf("\nEnter read key number (0 - 2 for NTAG413) (0 - 4 for NTAG424) or 14\n");
	scanf("%d%*c", &read_key_no_int);
	read_key_no = read_key_no_int;
	printf("\nEnter write key number (0 - 2 for NTAG413) (0 - 4 for NTAG424) or 14 or 15\n");
	scanf("%d%*c", &write_key_no_int);
	write_key_no = write_key_no_int;
	printf("\nEnter read_write key number (0 - 2 for NTAG413) (0 - 4 for NTAG424) or 0x14 or 0x15\n");
	scanf("%d%*c", &read_write_key_no_int);
	read_write_key_no = read_write_key_no_int;
	printf("\nEnter new change key number (0 - 2 for NTAG413) (0 - 4 for NTAG424)\n");
	scanf("%d%*c", &change_key_no_int);
	change_key_no = change_key_no_int;

	printf(" Select new communication mode\n");
	printf(" (1) - Plain mode\n");
	printf(" (2) - Macked mode\n");
	printf(" (3) - Enciphered mode\n");
	key = _getch();
	if(key == '1')
		new_communication_mode = 0;
	else if(key == '2')
		new_communication_mode = 1;
	else if(key == '3')
		new_communication_mode = 3;
	else
	{
		printf("Wrong choice\n");
		return;
	}

	printf(" Select authentication mode\n");
	printf(" (1) - Provided key\n");
	printf(" (2) - Internal key\n");
	key = _getch();
	if(key == '1')
	{
		internal_key = 0;
		printf("Enter change AES key (16 bytes hexadecimal)\n");
		if(!enter_aes_key(aes_key_ext))
			return;
	}
	else if(key == '2')
	{
		internal_key = 1;
		printf("Enter change AES internal key ordinal number (0 - 15)\n");
		scanf("%d%*c", &aes_internal_key_no_int);
		aes_internal_key_no = aes_internal_key_no_int;
	}
	else
	{
		printf("Wrong choice\n");
		return;
	}

	if(smd_data_file == 0)
	{
		//standard data file
		if(internal_key)
			status = nt4h_change_standard_file_settings(aes_internal_key_no, file_no, key_no, curr_communication_mode,
															new_communication_mode, read_key_no, write_key_no, read_write_key_no, change_key_no);
		else
			status = nt4h_change_standard_file_settings_pk(aes_key_ext, file_no, key_no, curr_communication_mode,
												new_communication_mode, read_key_no, write_key_no, read_write_key_no, change_key_no);
	}
	else
	{
		//Secure dynamic message file
		uint8_t uid_enable, read_ctr_enable, read_ctr_limit_enable, enc_file_data_enable;
		uint8_t meta_data_key_no, file_data_read_key_no, read_ctr_key_no;
		uint32_t uid_offset = 0, read_ctr_offset = 0, picc_data_offset = 0;
		uint32_t mac_input_offset = 0, enc_offset = 0, enc_length = 0, mac_offset = 0, read_ctr_limit = 0;
		int read_ctr_limit_int, meta_data_key_no_int, file_data_read_key_no_int, read_ctr_key_no_int;
		int uid_offset_int, read_ctr_offset_int, picc_data_offset_int;
		int mac_input_offset_int, enc_offset_int, enc_length_int, mac_offset_int;

		printf("UID mirroring enable (press Y or N)\n");
		key = _getch();
		if(key == 'Y' || key == 'y')
			uid_enable = 1;
		else
			uid_enable = 0;

		printf("Reading counter mirroring enable (press Y or N)\n");
		key = _getch();
		if(key == 'Y' || key == 'y')
			read_ctr_enable = 1;
		else
			read_ctr_enable = 0;

		printf("Reading counter limit enable (press Y or N)\n");
		key = _getch();
		if(key == 'Y' || key == 'y')
		{
			read_ctr_limit_enable = 1;

			printf("Enter reading counter limit\n");
			scanf("%d%*c", &read_ctr_limit_int);
			read_ctr_limit = read_ctr_limit_int;
		}
		else
			read_ctr_limit_enable = 0;

		printf("Encrypted part of file data enable NTAG424 only (press Y or N)\n");
		key = _getch();
		if(key == 'Y' || key == 'y')
			enc_file_data_enable = 1;
		else
			enc_file_data_enable = 0;

		printf("Enter SDM meta read access\n");
		printf("NTAG413 14 - plain PICC data, 15 -no PICC data\n");
		printf("NTAG424 0-4 encrypted PICC data, 14 - plain PICC data, 15 -no PICC data\n");
		scanf("%d%*c", &meta_data_key_no_int);
		meta_data_key_no = meta_data_key_no_int;

		printf("Enter SDM file data read access\n");
		printf("NTAG413 0-2 MAC exist, 15 no MAC\n");
		printf("NTAG424 0-4 MAC exist, 15 no MAC\n");
		scanf("%d%*c", &file_data_read_key_no_int);
		file_data_read_key_no = file_data_read_key_no_int;

		printf("Enter SDM reading counter access\n");
		printf("NTAG413 0-2 authentication, 14 - free, 15 - no access\n");
		printf("NTAG424 0-4 authentication, 14 - free, 15 - no access\n");
		scanf("%d%*c", &read_ctr_key_no_int);
		read_ctr_key_no = read_ctr_key_no_int;

		if(meta_data_key_no == 0x0E)
		{
			//plain PICC data
			if(uid_enable)
			{
				printf("Enter UID offset\n");
				scanf("%d%*c", &uid_offset_int);
				uid_offset = uid_offset_int;
			}

			if(read_ctr_enable)
			{
				printf("Enter reading counter offset\n");
				scanf("%d%*c", &read_ctr_offset_int);
				read_ctr_offset = read_ctr_offset_int;
			}
		}
		else if(meta_data_key_no <= 4)
		{
			//encrypted PICC data
			printf("Enter encrypted PICC data offset\n");
			scanf("%d%*c", &picc_data_offset_int);
			picc_data_offset = picc_data_offset_int;
		}

		if(file_data_read_key_no != 0x0F)
		{
			//MAC exist
			printf("Enter MAC input data offset\n");
			scanf("%d%*c", &mac_input_offset_int);
			mac_input_offset = mac_input_offset_int;

			if(enc_file_data_enable)
			{
				printf("Enter encrypted data offset NTAG424 only\n");
				scanf("%d%*c", &enc_offset_int);
				enc_offset = enc_offset_int;

				printf("Enter encrypted data length NTAG424 only\n");
				scanf("%d%*c", &enc_length_int);
				enc_length = enc_length_int;
			}

			printf("Enter MAC offset\n");
			scanf("%d%*c", &mac_offset_int);
			mac_offset = mac_offset_int;
		}

		if(internal_key)
			status = nt4h_change_sdm_file_settings(aes_internal_key_no, file_no, key_no, curr_communication_mode,
															new_communication_mode, read_key_no, write_key_no, read_write_key_no, change_key_no,
															uid_enable, read_ctr_enable, read_ctr_limit_enable, enc_file_data_enable,
															meta_data_key_no, file_data_read_key_no, read_ctr_key_no,
															uid_offset, read_ctr_offset, picc_data_offset,
															mac_input_offset, enc_offset, enc_length, mac_offset, read_ctr_limit);
		else
			status = nt4h_change_sdm_file_settings_pk(aes_key_ext, file_no, key_no, curr_communication_mode,
												new_communication_mode, read_key_no, write_key_no, read_write_key_no, change_key_no,
												uid_enable, read_ctr_enable, read_ctr_limit_enable, enc_file_data_enable,
												meta_data_key_no, file_data_read_key_no, read_ctr_key_no,
												uid_offset, read_ctr_offset, picc_data_offset,
												mac_input_offset, enc_offset, enc_length, mac_offset, read_ctr_limit);
	}

	if(status)
	{
		printf("\nSet file setting failed\n");
		printf("Error code = 0x%08X\n", status);
	}
	else
		printf("\nSet file setting successful");
}
//------------------------------------------------------------------------------
void get_uid(void)
{
	printf(" -------------------------------------------------------------------\n");
	printf("                             Get UID                                \n");
	printf("                           NTAG424 only                             \n");
	printf(" -------------------------------------------------------------------\n");

	UFR_STATUS status;
	char key;
	uint8_t auth_key[16];
	uint8_t key_no, uid[7];
	int key_no_int;
	uint8_t aes_internal_key_no;
	int aes_internal_key_no_int;

	printf("Enter key number (0 - 4)");
	scanf("%d%*c", &key_no_int);
	key_no = key_no_int;

	printf(" Select authentication mode\n");
	printf(" (1) - Provided key\n");
	printf(" (2) - Internal key\n");
	key = _getch();
	if(key == '1')
	{
		printf("Enter AES key (16 hexadecimal bytes)\n");
		if(!enter_aes_key(auth_key))
			return;

		status = nt4h_get_uid_pk(auth_key, key_no, uid);
	}
	else if(key == '2')
	{
		printf("Enter AES internal key ordinal number (0 - 15)");
		scanf("%d%*c", &aes_internal_key_no_int);
		aes_internal_key_no = aes_internal_key_no_int;

		status = nt4h_get_uid(aes_internal_key_no, key_no, uid);
	}
	else
	{
		printf("Wrong choice\n");
		return;
	}

	if(status)
	{
		printf("\nGet UID failed\n");
		printf("Error code = 0x%08X\n", status);
	}
	else
	{
		printf("\nGet UID successful");
		printf("\nUID = ");
		print_hex_ln(uid, 7, ":");
	}
}
//------------------------------------------------------------------------------
void set_rid(void)
{
	printf(" -------------------------------------------------------------------\n");
	printf("                           Set random ID                            \n");
	printf("                           NTAG424 only                             \n");
	printf(" -------------------------------------------------------------------\n");

	UFR_STATUS status;
	uint8_t aes_key_ext[16];
	char key;
	uint8_t aes_internal_key_no;
	int aes_internal_key_no_int;

	printf(" Select authentication mode\n");
	printf(" (1) - Provided key\n");
	printf(" (2) - Internal key\n");
	key = _getch();
	if(key == '1')
	{
		printf("Enter AES key (16 hexadecimal bytes)\n");
		if(!enter_aes_key(aes_key_ext))
			return;

		status = nt4h_set_rid_pk(aes_key_ext);
	}
	else if(key == '2')
	{
		printf("Enter AES internal key ordinal number (0 - 15)\n");
		scanf("%d%*c", &aes_internal_key_no_int);
		aes_internal_key_no = aes_internal_key_no_int;

		status = nt4h_set_rid(aes_internal_key_no);
	}
	else
	{
		printf("Wrong choice \n");
		return;
	}

	if(status)
	{
		printf("\nSet random ID failed\n");
		printf("Error code = 0x%08X\n", status);
	}
	else
		printf("\nSet random ID successful\n");
}
//------------------------------------------------------------------------------
void change_key(void)
{
	printf(" -------------------------------------------------------------------\n");
	printf("                            Change AES key                          \n");
	printf(" -------------------------------------------------------------------\n");

	UFR_STATUS status;
	uint8_t auth_key[16], key_no, new_key[16], old_key[16];
	int key_no_int;
	char key;
	uint8_t internal_key, aes_internal_key_no;
	int aes_internal_key_no_int;

	printf("Enter key number (0 - 4)\n");
	scanf("%d%*c", &key_no_int);
	key_no = key_no_int;

	printf(" Select authentication mode\n");
	printf(" (1) - Provided key\n");
	printf(" (2) - Internal key\n");
	key = _getch();
	if(key == '1')
	{
		internal_key = 0;
		printf("Enter master AES key (16 hexadecimal bytes)\n");
		if(!enter_aes_key(auth_key))
			return;
	}
	else if(key == '2')
	{
		internal_key = 1;
		printf("Enter AES internal key ordinal number (0 - 15)\n");
		scanf("%d%*c", &aes_internal_key_no_int);
		aes_internal_key_no = aes_internal_key_no_int;
	}
	else
	{
		printf("Wrong choice \n");
		return;
	}

	printf("Enter new AES key (16 hexadecimal bytes)\n");
	if(!enter_aes_key(new_key))
		return;

	printf("Enter old AES key (16 hexadecimal bytes)\n");
	if(!enter_aes_key(old_key))
		return;

	if(internal_key)
		status = nt4h_change_key(aes_internal_key_no, key_no, new_key, old_key);
	else
		status = nt4h_change_key_pk(auth_key, key_no, new_key, old_key);
	if(status)
	{
		printf("\nChange key failed\n");
		printf("Error code = 0x%08X\n", status);
	}
	else
		printf("\nChange key successful\n");
}
//------------------------------------------------------------------------------
void linear_read(void)
{
	printf(" -------------------------------------------------------------------\n");
	printf("                         Linear read                                \n");
	printf(" -------------------------------------------------------------------\n");

	UFR_STATUS status;
	uint8_t file_no, key_no, comm_mode;
	int file_no_int, key_no_int;
	char key;

	printf("Enter file number (NTAG413 1-2 NTAG424 1-3)\n");
	scanf("%d%*c", &file_no_int);
	file_no = file_no_int;

	printf("Enter key number (NTAG413 0-2 NTAG424 0-4\n");
	scanf("%d%*c", &key_no_int);
	key_no = key_no_int;

	printf(" Select communication mode\n");
	printf(" (1) - Plain mode\n");
	printf(" (2) - Macked mode\n");
	printf(" (3) - Enciphered mode\n");
	key = _getch();
	if(key == '1')
		comm_mode = 0;
	else if(key == '2')
		comm_mode = 1;
	else if(key == '3')
		comm_mode = 3;
	else
	{
		printf("Wrong choice\n");
		return;
	}

	status = nt4h_set_global_parameters(file_no, key_no, comm_mode);
	if(status)
	{
		printf("\nSet global parameters failed\n");
		printf("Error code = 0x%08X\n", status);
		return;
	}

	uint8_t data[256];
	uint16_t linear_address, length, bytes_returned;
	uint8_t aes_key[16];
	int linear_address_int, length_int;
	uint8_t aes_internal_key_no;
	int aes_internal_key_no_int;

	printf("Enter linear address\n");
	scanf("%d%*c", &linear_address_int);
	linear_address = linear_address_int;

	printf("Enter length\n");
	scanf("%d%*c", &length_int);
	length = length_int;

	printf(" Select authentication mode\n");
	printf(" (1) - Provided key\n");
	printf(" (2) - Internal key\n");
	printf(" (3) - No authentication\n");
	key = _getch();

	if(key == '1')
	{
		printf("Enter AES key\n");
		if(!enter_aes_key(aes_key))
			return;

		status = LinearRead_PK(data, linear_address, length, &bytes_returned, T4T_PK_PWD_AUTH, aes_key);
	}
	else if(key == '2')
	{
		printf("Enter AES internal key ordinal number (0 - 15)\n");
		scanf("%d%*c", &aes_internal_key_no_int);
		aes_internal_key_no = aes_internal_key_no_int;

		status = LinearRead(data, linear_address, length, &bytes_returned, T4T_RKA_PWD_AUTH, aes_internal_key_no);
	}
	else
	{
		status = LinearRead(data, linear_address, length, &bytes_returned, T4T_WITHOUT_PWD_AUTH, 0);
	}

	if(status)
	{
		printf("\nLinear read failed\n");
		printf("Error code = 0x%08X\n", status);
	}
	else
	{
		printf("\nLiear read successful\n");
		printf("Hexadecimal: \n");
		print_hex_ln(data, length, ":");
		printf("ASCI: \n");
		printf("%s\n", data);
	}
}
//------------------------------------------------------------------------------
void linear_write(void)
{
	printf(" -------------------------------------------------------------------\n");
	printf("                         Linear write                               \n");
	printf(" -------------------------------------------------------------------\n");

	UFR_STATUS status;
		uint8_t file_no, key_no, comm_mode;
		int file_no_int, key_no_int;
		char key;

		printf("Enter file number (NTAG413 1-2 NTAG424 1-3)\n");
		scanf("%d%*c", &file_no_int);
		file_no = file_no_int;

		printf("Enter key number (NTAG413 0-2 NTAG424 0-4\n");
		scanf("%d%*c", &key_no_int);
		key_no = key_no_int;

		printf(" Select communication mode\n");
		printf(" (1) - Plain mode\n");
		printf(" (2) - Macked mode\n");
		printf(" (3) - Enciphered mode\n");
		key = _getch();
		if(key == '1')
			comm_mode = 0;
		else if(key == '2')
			comm_mode = 1;
		else if(key == '3')
			comm_mode = 3;
		else
		{
			printf("Wrong choice\n");
			return;
		}

		status = nt4h_set_global_parameters(file_no, key_no, comm_mode);
		if(status)
		{
			printf("\nSet global parameters failed\n");
			printf("Error code = 0x%08X\n", status);
			return;
		}

		uint8_t data[256];
		uint16_t linear_address, length, bytes_returned;
		uint8_t aes_key[16];
		int linear_address_int;
		uint8_t aes_internal_key_no;
		int aes_internal_key_no_int;

		printf("Enter linear address\n");
		scanf("%d%*c", &linear_address_int);
		linear_address = linear_address_int;

		printf("Enter data\n");
		if(!enter_linear_data(data, &length))
		{
			printf("Wrong choice\n");
			return;
		}

		printf(" Select authentication mode\n");
		printf(" (1) - Provided key\n");
		printf(" (2) - Internal key\n");
		printf(" (3) - No authentication\n");
		key = _getch();

		if(key == '1')
		{
			printf("Enter AES key\n");
			if(!enter_aes_key(aes_key))
				return;

			status = LinearWrite_PK(data, linear_address, length, &bytes_returned, T4T_PK_PWD_AUTH, aes_key);
		}
		else if(key == '2')
		{
			printf("Enter AES internal key ordinal number (0 - 15)\n");
			scanf("%d%*c", &aes_internal_key_no_int);
			aes_internal_key_no = aes_internal_key_no_int;

			status = LinearWrite(data, linear_address, length, &bytes_returned, T4T_RKA_PWD_AUTH, aes_internal_key_no);
		}
		else
		{
			status = LinearWrite(data, linear_address, length, &bytes_returned, T4T_WITHOUT_PWD_AUTH, 0);
		}

		if(status)
		{
			printf("\nLinear write failed\n");
			printf("Error code = 0x%08X\n", status);
		}
		else
		{
			printf("\nLiear write successful\n");
		}
}
//------------------------------------------------------------------------------
void sdm_read(void)
{
	printf(" -------------------------------------------------------------------\n");
	printf("                 Secure dynamic message read                        \n");
	printf("   File number = 2, free read access, plain communication mode      \n");
	printf(" -------------------------------------------------------------------\n");

	UFR_STATUS status;
	uint8_t file_no, file_type, communication_mode, sdm_enable;
	uint8_t read_key_no, write_key_no, read_write_key_no, change_key_no;
	uint8_t uid_enable, read_ctr_enable, read_ctr_limit_enable, enc_file_data_enable;
	uint8_t meta_data_key_no, file_data_read_key_no, read_ctr_key_no;
	uint32_t file_size = 0, uid_offset = 0, read_ctr_offset = 0, picc_data_offset = 0;
	uint32_t mac_input_offset = 0, enc_offset = 0, enc_length = 0, mac_offset = 0, read_ctr_limit = 0;
	uint8_t data[257];
	uint16_t ret_bytes;
	uint8_t picc_data_tag, uid[7];
	uint32_t sdm_read_cnt = 0;
	uint8_t file_data_aes_key[16];

	file_no = 2;
	status = nt4h_get_file_settings(file_no, &file_type, &communication_mode, &sdm_enable, &file_size,
									&read_key_no, &write_key_no, &read_write_key_no, &change_key_no,
									&uid_enable, &read_ctr_enable, &read_ctr_limit_enable, &enc_file_data_enable,
									&meta_data_key_no, &file_data_read_key_no, &read_ctr_key_no,
									&uid_offset, &read_ctr_offset, &picc_data_offset,
									&mac_input_offset, &enc_offset, &enc_length, &mac_offset, &read_ctr_limit);
	if(status)
	{
		printf("\nGet file settings failed");
		printf("Error code = 0x%08X\n", status);
		return;
	}
	//check file parameters
	if(!(communication_mode == 0 && sdm_enable && read_key_no == 0x0E))
	{
		printf("\nFile is not in SDM mode\n");
		return;
	}

	//read file data
	memset(data, 0, 257);

	status = nt4h_set_global_parameters(2, 0x0E, 0);
	if(status)
	{
		printf("\nGlobal parameters setting failed");
		printf("Error code = 0x%08X\n", status);
		return;
	}

	status = LinearRead(data, 0, file_size, &ret_bytes, T4T_WITHOUT_PWD_AUTH, 0);
	if(status)
	{
		printf("\nReading data failed");
		printf("Error code = 0x%08X\n", status);
		return;
	}

	printf("Raw hexadecimal data: \n");
	print_hex_ln(data, file_size, ":");
	printf("NDEF file context: %s\n", &data[7]);
	if(meta_data_key_no <= 4)
	{
		//PICC encrypted data
		uint8_t enc_picc_data[33];
		uint8_t picc_data[16];
		uint8_t meta_data_aes_key[16];

		enc_picc_data[32] = 0;
		memcpy(enc_picc_data, &data[picc_data_offset], 32);
		printf("\nPICC encrypted data: %s\n", enc_picc_data);
		hex2bin(picc_data, (const char *)enc_picc_data);

		printf("Enter meta data AES key (16 hexadecimal bytes)\n");
		if(!enter_aes_key(meta_data_aes_key))
			return;

		status = nt4h_decrypt_picc_data(picc_data, meta_data_aes_key, &picc_data_tag, uid, &sdm_read_cnt);
		if(status)
		{
			printf("\nPICC data decrypting failed");
			printf("Error code = 0x%08X\n", status);
			return;
		}

		printf("PICC data decrypted successful\n");
		if(picc_data_tag & 0x80)
		{
			printf("UID = ");
			print_hex_ln(uid, 7, ":");
		}
		if(picc_data_tag & 0x40)
		{
			printf("Reading counter = %d\n", sdm_read_cnt);
		}
	}
	if(meta_data_key_no == 0x0E)
	{
		//PICC data doesn't encrypted
		uint8_t ascii_uid[15];
		uint8_t ascii_sdm_read_cnt[7];
		uint8_t sdm_read_cnt_array[3];
		uint8_t temp;

		if(uid_enable)
		{
			ascii_uid[14] = 0;
			memcpy(ascii_uid, &data[uid_offset], 14);
			printf("\nUID: %s\n", ascii_uid);
			hex2bin(uid, (const char *)ascii_uid);
		}

		if(read_ctr_enable)
		{
			ascii_sdm_read_cnt[6] = 0;
			memcpy(ascii_sdm_read_cnt, &data[read_ctr_offset], 6);
			printf("\nSDM reading counter: %s\n", ascii_sdm_read_cnt);
			hex2bin(sdm_read_cnt_array, (const char *)ascii_sdm_read_cnt);
			temp = sdm_read_cnt_array[2];
			sdm_read_cnt_array[2] = sdm_read_cnt_array[0];
			sdm_read_cnt_array[0] = temp;
			memcpy(&sdm_read_cnt, sdm_read_cnt_array, 3);
		}
	}

	if(enc_file_data_enable)
	{
		//Part of file data encrypted
		uint8_t enc_file_data[256];
		uint8_t file_data[128];

		memset(enc_file_data, 0, 256);
		memset(file_data, 0, 128);

		memcpy(enc_file_data, &data[enc_offset], enc_length);
		printf("\nEncrypted part of file data: %s\n", enc_file_data);
		hex2bin(file_data, (const char *)enc_file_data);

		printf("Enter file data read AES key (16 hexadecimal bytes)\n");
		if(!enter_aes_key(file_data_aes_key))
			return;

		status = nt4h_decrypt_sdm_enc_file_data(sdm_read_cnt, uid, file_data_aes_key, file_data, enc_length / 2);
		if(status)
		{
			printf("\nPart of file data decrypting failed");
			printf("Error code = 0x%08X\n", status);
			return;
		}

		printf("Part of file data decrypted successful\n");
		printf("Part of file data = %s\n", file_data);
	}

	if(file_data_read_key_no != 0x0F)
	{
		//MAC exist
		uint8_t ascii_mac_data[17];
		uint8_t mac[8];
		uint8_t ascii_mac_in[256];
		uint8_t mac_in_len;

		if(!enc_file_data_enable)
		{
			printf("Enter file data read AES key (16 hexadecimal bytes)\n");
			if(!enter_aes_key(file_data_aes_key))
				return;
		}

		ascii_mac_data[16] = 0;
		memcpy(ascii_mac_data, &data[mac_offset], 16);
		printf("\nASCI MAC data: %s\n", ascii_mac_data);
		hex2bin(mac, (const char *)ascii_mac_data);

		mac_in_len = mac_offset - mac_input_offset;
		if(mac_in_len)
		{
			memcpy(ascii_mac_in, &data[mac_input_offset], mac_in_len);
			printf("ASCI MAC input data: %s\n", ascii_mac_in);
		}

		status = nt4h_check_sdm_mac(sdm_read_cnt, uid, file_data_aes_key, ascii_mac_in, mac_in_len, mac);
		if(status)
		{
			printf("\nMAC is not correct\n");
			printf("Error code = 0x%08X\n", status);
			return;
		}

		printf("MAC is correct\n");
	}
}
//------------------------------------------------------------------------------

void sdm_write(void)
{
	printf(" -------------------------------------------------------------------\n");
	printf("                 Secure dynamic message write                       \n");
	printf("   File number = 2, free read access, plain communication mode      \n");
	printf(" -------------------------------------------------------------------\n");

	UFR_STATUS status;
	uint8_t aes_key_ext[16];
	uint8_t file_no, key_no, communication_mode;
	uint8_t read_key_no, write_key_no, read_write_key_no, change_key_no;
	uint8_t internal_key, aes_internal_key_no;
	int key_no_int;
	int write_key_no_int, read_write_key_no_int, change_key_no_int;
	int aes_internal_key_no_int;
	uint8_t uid_enable, read_ctr_enable, read_ctr_limit_enable, enc_file_data_enable;
	uint8_t meta_data_key_no, file_data_read_key_no, read_ctr_key_no;
	uint32_t uid_offset = 0, read_ctr_offset = 0, picc_data_offset = 0;
	uint32_t mac_input_offset = 0, enc_offset = 0, enc_length = 0, mac_offset = 0, read_ctr_limit = 0;
	int read_ctr_limit_int, meta_data_key_no_int, file_data_read_key_no_int, read_ctr_key_no_int;

	char key;

	file_no = 2;	//NDEF file
	communication_mode = 0; //plain
	read_key_no = 0x0E; //free read access

	//file access key numbers
	printf("\nEnter change key number (0 - 2 for NTAG413) (0 - 4 for NTAG424)\n");
	scanf("%d%*c", &key_no_int);
	key_no = key_no_int;

	printf("\nEnter write key number (0 - 2 for NTAG413) (0 - 4 for NTAG424) or 14 or 15\n");
	scanf("%d%*c", &write_key_no_int);
	write_key_no = write_key_no_int;

	printf("\nEnter read_write key number (0 - 2 for NTAG413) (0 - 4 for NTAG424) or 0x14 or 0x15\n");
	scanf("%d%*c", &read_write_key_no_int);
	read_write_key_no = read_write_key_no_int;

	printf("\nEnter new change key number (0 - 2 for NTAG413) (0 - 4 for NTAG424)\n");
	scanf("%d%*c", &change_key_no_int);
	change_key_no = change_key_no_int;

	printf("Does PICC data (UID, SDM reading counter) exist? (press Y or N)\n");
	key = _getch();
	if(key == 'Y' || key == 'y')
	{
		printf("NTAG424 only\n");
		printf("Does PICC data encrypted? (press Y or N)\n");
		key = _getch();
		if(key == 'Y' || key == 'y')
		{
			//encrypted PICC data
			printf("Enter SDM meta read access\n");
			printf("NTAG424 0-4 encrypted PICC data\n");
			scanf("%d%*c", &meta_data_key_no_int);
			meta_data_key_no = meta_data_key_no_int;
		}
		else
		{
			meta_data_key_no = 0x0E; //plain PICC data
		}

		printf("UID mirroring enable (press Y or N)\n");
		key = _getch();
		if(key == 'Y' || key == 'y')
			uid_enable = 1;
		else
			uid_enable = 0;

		printf("Reading counter mirroring enable (press Y or N)\n");
		key = _getch();
		if(key == 'Y' || key == 'y')
			read_ctr_enable = 1;
		else
			read_ctr_enable = 0;
	}
	else
	{
		meta_data_key_no = 0x0F; //no PICC data
	}

	printf("Encrypted part of file data enable NTAG424 only (press Y or N)\n");
	key = _getch();
	if(key == 'Y' || key == 'y')
		enc_file_data_enable = 1;
	else
		enc_file_data_enable = 0;

	printf("Does MAC exist? (press Y or N)\n");
	key = _getch();
	if(key == 'Y' || key == 'y')
	{
		printf("Enter SDM file data read access\n");
		printf("NTAG413 0-2 MAC exist\n");
		printf("NTAG424 0-4 MAC exist\n");
		scanf("%d%*c", &file_data_read_key_no_int);
		file_data_read_key_no = file_data_read_key_no_int;
	}
	else
	{
		file_data_read_key_no = 0x0E; //no MAC
	}

	printf("SDM reading counter limit enable (press Y or N)\n");
	key = _getch();
	if(key == 'Y' || key == 'y')
	{
		read_ctr_limit_enable = 1;

		printf("Enter SDM reading counter limit\n");
		scanf("%d%*c", &read_ctr_limit_int);
		read_ctr_limit = read_ctr_limit_int;
	}
	else
		read_ctr_limit_enable = 0;

	printf("Enter SDM reading counter access\n");
	printf("NTAG413 0-2 authentication, 14 - free, 15 - no access\n");
	printf("NTAG424 0-4 authentication, 14 - free, 15 - no access\n");
	scanf("%d%*c", &read_ctr_key_no_int);
	read_ctr_key_no = read_ctr_key_no_int;

	//NDEF message creation
	uint8_t url[100];
	uint8_t url_len;
	uint8_t ndef_data[256];
	uint8_t ndef_len;
#define NDEF_HEADER_LEN	7
	uint8_t ndef_header[] = {0x00, 0x00, 0xD1, 0x01, 0x00, 0x55, 0x00};

	memset(ndef_data, 0, 256);

	printf("Enter URL (for example http://www.test.com/nt4h)\n");
	enter_ascii_data(url, &url_len);

	memcpy(&ndef_data[NDEF_HEADER_LEN], url, url_len);
	ndef_len = url_len;
	ndef_data[url_len + NDEF_HEADER_LEN] = '?';
	url_len++;

	if(meta_data_key_no != 0x0F)
	{
		//PICC data exist
		if(meta_data_key_no == 0x0E)
		{
			//Plain data
			if(uid_enable)
			{
				//UID mirroring enabled
				ndef_data[url_len + NDEF_HEADER_LEN] = 'u';
				ndef_data[url_len + NDEF_HEADER_LEN + 1] = '=';
				memset(&ndef_data[url_len + NDEF_HEADER_LEN + 2], '0', 14);
				uid_offset = url_len + NDEF_HEADER_LEN + 2;
				url_len += 16;
			}

			if(read_ctr_enable)
			{
				//SDM readin counter mirroring enabled
				ndef_data[url_len + NDEF_HEADER_LEN] = 'c';
				ndef_data[url_len + NDEF_HEADER_LEN + 1] = '=';
				memset(&ndef_data[url_len + NDEF_HEADER_LEN + 2], '0', 6);
				read_ctr_offset = url_len + NDEF_HEADER_LEN + 2;
				url_len += 8;
			}
		}
		else
		{
			//Encrypted PICC data
			ndef_data[url_len + NDEF_HEADER_LEN] = 'p';
			ndef_data[url_len + NDEF_HEADER_LEN + 1] = '=';
			memset(&ndef_data[url_len + NDEF_HEADER_LEN + 2], '0', 32);
			picc_data_offset = url_len + NDEF_HEADER_LEN + 2;
			url_len += 34;
		}
	}


	if(file_data_read_key_no != 0x0E)
	{
		//MAC exist
		int mac_input_ctr_int;
		uint8_t mac_input_ctr = 0;

		printf("Enter additional number of characters for MAC calculation\n");
		printf("counted to left from MAC position (default 0 no additional data)\n");
		scanf("%d%*c", &mac_input_ctr_int);
		mac_input_ctr = mac_input_ctr_int;

		if(enc_file_data_enable)
		{
			//NTAG424 only
			//Encrypted part of file data
			uint8_t enc_file_data[100];
			uint8_t enc_file_len, total_enc_file_len;

			printf("Enter data for encryption\n");
			enter_ascii_data(enc_file_data, &enc_file_len);

			ndef_data[url_len + NDEF_HEADER_LEN] = 'e';
			ndef_data[url_len + NDEF_HEADER_LEN + 1] = '=';
			total_enc_file_len = enc_file_len;
			if(enc_file_len % 16)
				total_enc_file_len += 16 - enc_file_len % 16;
			total_enc_file_len *= 2;
			memset(&ndef_data[url_len + NDEF_HEADER_LEN + 2], 0, total_enc_file_len);
			memcpy(&ndef_data[url_len + NDEF_HEADER_LEN + 2], enc_file_data, enc_file_len);

			enc_offset = url_len + NDEF_HEADER_LEN + 2;
			mac_input_offset = enc_offset - mac_input_ctr;
			enc_length = total_enc_file_len;
			url_len += total_enc_file_len + 2;
		}

		ndef_data[url_len + NDEF_HEADER_LEN] = 'm';
		ndef_data[url_len + NDEF_HEADER_LEN + 1] = '=';
		memset(&ndef_data[url_len + NDEF_HEADER_LEN + 2], 0, 16);
		if(!enc_file_data_enable)
			mac_input_offset = url_len + NDEF_HEADER_LEN + 2 - mac_input_ctr;
		mac_offset = url_len + NDEF_HEADER_LEN + 2;
		url_len += 18;
	}

	ndef_header[1] = url_len + 5;
	ndef_header[4] = url_len + 1;
	memcpy(ndef_data, ndef_header, NDEF_HEADER_LEN);

	printf("Hexadecimal file data with NDEF header\n");
	print_hex_ln(ndef_data, url_len + NDEF_HEADER_LEN, ":");
	printf("NDEF message\n");
	printf("%s\n", &ndef_data[NDEF_HEADER_LEN]);
	ndef_len = url_len + NDEF_HEADER_LEN;

	printf("\n Change setting of file number 2\n");
	printf(" File change AES key\n");
	printf(" Select authentication mode\n");
	printf(" (1) - Provided key\n");
	printf(" (2) - Internal key\n");
	key = _getch();
	if(key == '1')
	{
		internal_key = 0;
		printf("Enter change AES key (16 bytes hexadecimal)\n");
		if(!enter_aes_key(aes_key_ext))
			return;
	}
	else if(key == '2')
	{
		internal_key = 1;
		printf("Enter change AES internal key ordinal number (0 - 15)\n");
		scanf("%d%*c", &aes_internal_key_no_int);
		aes_internal_key_no = aes_internal_key_no_int;
	}
	else
	{
		printf("Wrong choice\n");
		return;
	}

	if(internal_key)
		status = nt4h_change_sdm_file_settings(aes_internal_key_no, file_no, key_no, 3,
														communication_mode, read_key_no, write_key_no, read_write_key_no, change_key_no,
														uid_enable, read_ctr_enable, read_ctr_limit_enable, enc_file_data_enable,
														meta_data_key_no, file_data_read_key_no, read_ctr_key_no,
														uid_offset, read_ctr_offset, picc_data_offset,
														mac_input_offset, enc_offset, enc_length, mac_offset, read_ctr_limit);
	else
		status = nt4h_change_sdm_file_settings_pk(aes_key_ext, file_no, key_no, 3,
											communication_mode, read_key_no, write_key_no, read_write_key_no, change_key_no,
											uid_enable, read_ctr_enable, read_ctr_limit_enable, enc_file_data_enable,
											meta_data_key_no, file_data_read_key_no, read_ctr_key_no,
											uid_offset, read_ctr_offset, picc_data_offset,
											mac_input_offset, enc_offset, enc_length, mac_offset, read_ctr_limit);

	if(status)
	{
		printf("\nSet file setting failed\n");
		printf("Error code = 0x%08X\n", status);
		return;
	}

	uint16_t bytes_written;

	printf("\n Write NDEF into file number 2\n");
	status = nt4h_set_global_parameters(file_no, write_key_no, communication_mode);
	if(status)
	{
		printf("\nSet global parameters failed\n");
		printf("Error code = 0x%08X\n", status);
		return;
	}

	if(write_key_no == 0x0E)
	{
		status = LinearWrite(ndef_data, 0, ndef_len + 1, &bytes_written, T4T_WITHOUT_PWD_AUTH, 0);
	}
	else
	{
		printf(" File write AES key\n");
		printf(" Select authentication mode\n");
		printf(" (1) - Provided key\n");
		printf(" (2) - Internal key\n");
		key = _getch();
		if(key == '1')
		{
			internal_key = 0;
			printf("Enter change AES key (16 bytes hexadecimal)\n");
			if(!enter_aes_key(aes_key_ext))
				return;
		}
		else if(key == '2')
		{
			internal_key = 1;
			printf("Enter change AES internal key ordinal number (0 - 15)\n");
			scanf("%d%*c", &aes_internal_key_no_int);
			aes_internal_key_no = aes_internal_key_no_int;
		}
		else
		{
			printf("Wrong choice\n");
			return;
		}

		if(key == '1')
			status = LinearWrite_PK(ndef_data, 0, ndef_len + 1, &bytes_written, T4T_PK_PWD_AUTH, aes_key_ext);
		else
			status = LinearWrite(ndef_data, 0, ndef_len + 1, &bytes_written, T4T_RKA_PWD_AUTH, aes_internal_key_no);
	}

	if(status)
	{
		printf("\nLinear write failed\n");
		printf("Error code = 0x%08X\n", status);
	}
	else
		printf("Secure dynamic message write successful\n");
}
//------------------------------------------------------------------------------
void read_sdm_counter(void)
{
	printf(" -------------------------------------------------------------------\n");
	printf("                     Get SDM reading counter                        \n");
	printf(" -------------------------------------------------------------------\n");

	UFR_STATUS status;
	uint8_t auth_key[16];
	uint8_t file_no, key_no;
	uint32_t sdm_read_ctr;
	int file_no_int, key_no_int;
	char key;
	uint8_t aes_internal_key_no;
	int aes_internal_key_no_int;

	printf("Enter file number (NTAG413 2 NTAG424 2-3)\n");
	scanf("%d%*c", &file_no_int);
	file_no = file_no_int;

	printf(" Select authentication mode\n");
	printf(" (1) - Provided key\n");
	printf(" (2) - Internal key\n");
	printf(" (3) - No authentication\n");
	key = _getch();

	if(key == '1')
	{
		printf("Enter key number (NTAG413 0-2 NTAG424 0-4\n");
		scanf("%d%*c", &key_no_int);
		key_no = key_no_int;

		printf("Enter AES key (16 hexadecimal bytes)\n");
		if(!enter_aes_key(auth_key))
			return;

		status = nt4h_get_sdm_ctr_pk(auth_key, file_no, key_no, &sdm_read_ctr);
	}
	else if(key == '2')
	{
		printf("Enter key number (NTAG413 0-2 NTAG424 0-4\n");
		scanf("%d%*c", &key_no_int);
		key_no = key_no_int;

		printf("Enter AES internal key ordinal number (0 - 15)\n");
		scanf("%d%*c", &aes_internal_key_no_int);
		aes_internal_key_no = aes_internal_key_no_int;

		status = nt4h_get_sdm_ctr(aes_internal_key_no, file_no, key_no, &sdm_read_ctr);
	}
	else if(key == '3')
	{
		status = nt4h_get_sdm_ctr_no_auth(file_no, &sdm_read_ctr);
	}
	else
	{
		printf("Wrong choice\n");
		return;
	}


	if(status)
	{
		printf("\nGet SDM reading counter failed\n");
		printf("Error code = 0x%08X\n", status);
		return;
	}

	printf("\nSDM reading counter = %d\n", sdm_read_ctr);
}
//------------------------------------------------------------------------------

void store_key(void)
{
	printf(" -------------------------------------------------------------------\n");
	printf("                     Store AES key into reader                      \n");
	printf(" -------------------------------------------------------------------\n");

	UFR_STATUS status;
	uint8_t aes_key[16], password[16], key_index;
	uint16_t pass_len;
	char key;
	int key_index_int;

	printf(" (1) - AES keys \n"
		   " (2) - Unlock reader \n"
		   " (3) - Lock reader \n");
	while (!_kbhit())
		;
	key = _getch();

	if(key == '1')
	{
		printf("\nEnter AES key\n");
		if(!enter_aes_key(aes_key))
		{
			printf("\nError while key entry\n");
			return;
		}

		printf("\nEnter key index (0 - 15)\n");
		scanf("%d%*c", &key_index_int);
		key_index = key_index_int;

		status = uFR_int_DesfireWriteAesKey(key_index, aes_key);
		if(status)
		{
			printf("\nWriting key into reader failed\n");
			printf("Error code = %02X\n", status);
		}
		else
			printf("\nKey written into reader\n");
	}
	else if(key == '2')
	{
		printf("\nEnter password of 8 bytes\n");

		if(!enter_linear_data(password, &pass_len))
		{
			printf("\nError while password entry\n");
			return;
		}
		if(pass_len != 8)
		{
			printf("\nPassword length is wrong\n");
			return;
		}

		status = ReaderKeysUnlock(password);
		if(status)
		{
			printf("\nUnlock keys error\n");
			printf("Error code = %02X\n", status);
		}
		else
			printf("\nReader keys are unlocked\n");
	}
	else if(key == '3')
	{
		printf("\nEnter password of 8 bytes\n");

		if(!enter_linear_data(password, &pass_len))
		{
			printf("\nError while password entry\n");
			return;
		}
		if(pass_len != 8)
		{
			printf("\nPassword length is wrong\n");
			return;
		}

		status = ReaderKeysLock(password);
		if(status)
		{
			printf("\nLock keys error\n");
			printf("Error code = %02X\n", status);
		}
		else
			printf("\nReader keys are locked\n");
	}
	else
		printf("\nWrong choice\n");

	return;
}
//------------------------------------------------------------------------------
