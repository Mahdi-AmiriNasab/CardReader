/* USER CODE BEGIN Header */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usb_device.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "usbd_cdc_if.h"
#include "PN532.h"
#include "stdio.h"
#include "UserManager.h"
#include "Classic.h"
//#include "Adafruit_PN532.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

// When using Desfire EV1 cards a 16 byte data block is stored in the card's EEPROM memory 
// that can only be read with the application master key.
// IMPORTANT: After changing this compiler switch, please execute the CLEAR command!
#define USE_DESFIRE   true


//define file size for each std file

#define FileSize 	13

#define Delay_Blink  10

#define USER1 to_save_mahdi
#define USER2 to_save_mehran

#if FileSize > 13
	#error "FileSize value more than 13 causes hardware disaster";
#endif


#ifndef SELECT_USER
	#define SELECT_USER USER1
#endif


#if USE_DESFIRE

    #if USE_AES
        #define DESFIRE_KEY_TYPE   AES
        #define DEFAULT_APP_KEY    rfid.AES_DEFAULT_KEY
    #else
        #define DESFIRE_KEY_TYPE   DES
		#ifdef KEY_24
			#define DEFAULT_APP_KEY    rfid.DES3_DEFAULT_KEY
		#else
			#define DEFAULT_APP_KEY    rfid.DES2_DEFAULT_KEY
		#endif
	#endif
    
    #include "Desfire.h"
    #include "Secrets.h"
    #include "Buffer.h"
    Desfire          rfid;//(irq_pin ,reset_pin); // The class instance that communicates with Mifare Desfire cards   
    DESFIRE_KEY_TYPE gi_PiccMasterKey;
#else
    #include "Classic.h"
    Classic          gi_PN532; // The class instance that communicates with Mifare Classic cards


	/*
    // This compiler switch defines if you use AES (128 bit) or DES (168 bit) for the PICC master key and the application master key.
    // Cryptographers say that AES is better.
    // But the disadvantage of AES encryption is that it increases the power consumption of the card more than DES.
    // The maximum read distance is 5,3 cm when using 3DES keys and 4,0 cm when using AES keys.
    // (When USE_DESFIRE == false the same Desfire card allows a distance of 6,3 cm.)
    // If the card is too far away from the antenna you get a timeout error at the moment when the Authenticate command is executed.
    // IMPORTANT: Before changing this compiler switch, please execute the RESTORE command on all personalized cards!*/
	#define USE_AES   false
	/*
    // This define should normally be zero
    // If you want to run the selftest (only available if USE_DESFIRE == true) you must set this to a value > 0.
    // Then you can enter TEST into the terminal to execute a selftest that tests ALL functions in the Desfire class.
    // The value that you can specify here is 1 or 2 which will be the debug level for the selftest.
    // At level 2 you see additionally the CMAC and the data sent to and received from the card.*/
    #define COMPILE_SELFTEST  0
    /*
    // This define should normally be false
    // If this is true you can use Classic cards / keyfobs additionally to Desfire cards.
    // This means that the code is compiled for Defire cards, but when a Classic card is detected it will also work.
    // This mode is not recommended because Classic cards do not offer the same security as Desfire cards.*/
    #define ALLOW_ALSO_CLASSIC   false
#endif

	
DBG Serial;
kCard mycard;
eCardType mycard_type;
uint8_t config = 0;
uint8_t IC, VersionHi, VersionLo, Flags;

// global variables
char       gs8_CommandBuffer[500];    // Stores commands typed by the user via Terminal and the password
uint32_t   gu32_CommandPos   = 0;     // Index in gs8_CommandBuffer
uint64_t   gu64_LastPasswd   = 0;     // Timestamp when the user has enetered the password successfully
uint64_t   gu64_LastID       = 0;     // The last card UID that has been read by the RFID reader  
bool       gb_InitSuccess    = false; // true if the PN532 has been initialized successfully


//PN532 cls(irq_pin, reset_pin);

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

SPI_HandleTypeDef hspi1;

UART_HandleTypeDef huart2;
/* USER CODE BEGIN PV */
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */



#if USE_DESFIRE

// If the card is personalized -> authenticate with SECRET_PICC_MASTER_KEY,
// otherwise authenticate with the factory default DES key.
bool AuthenticatePICC(byte* pu8_KeyVersion)
{
    if (!rfid.SelectApplication(0x000000)) // PICC level
        return false;

    if (!rfid.GetKeyVersion(0, pu8_KeyVersion)) // Get version of PICC master key
        return false;

    // The factory default key has version 0, while a personalized card has key version CARD_KEY_VERSION
    if (*pu8_KeyVersion == CARD_KEY_VERSION)
    {
        if (!rfid.Authenticate(0, &gi_PiccMasterKey))
            return false;
		Utils::Print("Athentication has done in key version which set by user", LF);
    }
    else // The card is still in factory default state
    {
        if (!rfid.Authenticate(0, &rfid.DES2_DEFAULT_KEY))
            return false;
		Utils::Print("Athentication has done in key version which set default by factory",LF);

    }
    return true;
}

// Generate two dynamic secrets: the Application master key (AES 16 byte or DES 24 byte) and the 16 byte StoreValue.
// Both are derived from the 7 byte card UID and the the user name + random data stored in EEPROM using two 24 byte 3K3DES keys.
// This function takes only 6 milliseconds to do the cryptographic calculations.
bool GenerateDesfireSecrets(kUser* pk_User, DESFireKey* pi_AppMasterKey, byte u8_StoreValue[16])
{
    // The buffer is initialized to zero here
    byte u8_Data[24] = {0}; 

    // Copy the 7 byte card UID into the buffer
    memcpy(u8_Data, pk_User->ID.u8, 7);

    // XOR the user name and the random data that are stored in EEPROM over the buffer.
    // s8_Name[NAME_BUF_SIZE] contains for example { 'P', 'e', 't', 'e', 'r', 0, 0xDE, 0x45, 0x70, 0x5A, 0xF9, 0x11, 0xAB }
    int B=0;
    for (int N=0; N<NAME_BUF_SIZE; N++)
    {
        u8_Data[B++] ^= pk_User->s8_Name[N];
        if (B > 15) B = 0; // Fill the first 16 bytes of u8_Data, the rest remains zero.
    }
	
	#ifdef KEY_24
    byte u8_AppMasterKey[24];
	#else
	byte u8_AppMasterKey[16];
	#endif
    DES i_3KDes;
    if (!i_3KDes.SetKeyData(SECRET_APPLICATION_KEY, sizeof(SECRET_APPLICATION_KEY), 0) || // set a 24 byte key (168 bit)
        !i_3KDes.CryptDataCBC(CBC_SEND, KEY_ENCIPHER, u8_AppMasterKey, u8_Data, 24))
        return false;
    
    if (!i_3KDes.SetKeyData(SECRET_STORE_VALUE_KEY, sizeof(SECRET_STORE_VALUE_KEY), 0) || // set a 24 byte key (168 bit)
        !i_3KDes.CryptDataCBC(CBC_SEND, KEY_ENCIPHER, u8_StoreValue, u8_Data, 16))
        return false;

    // If the key is an AES key only the first 16 bytes will be used
    if (!pi_AppMasterKey->SetKeyData(u8_AppMasterKey, sizeof(u8_AppMasterKey), CARD_KEY_VERSION))
        return false;

    return true;
}

// Check that the data stored on the card is the same as the secret generated by GenerateDesfireSecrets()
bool CheckDesfireSecret(kUser* pk_User)
{
    DESFIRE_KEY_TYPE i_AppMasterKey;
    byte u8_StoreValue[16];
    if (!GenerateDesfireSecrets(pk_User, &i_AppMasterKey, u8_StoreValue))
        return false;

    if (!rfid.SelectApplication(0x000000)) // PICC level
        return false;

    byte u8_Version; 
    if (!rfid.GetKeyVersion(0, &u8_Version))
        return false;

    // The factory default key has version 0, while a personalized card has key version CARD_KEY_VERSION
    if (u8_Version != CARD_KEY_VERSION)
        return false;

    if (!rfid.SelectApplication(CARD_APPLICATION_ID))
        return false;

    if (!rfid.Authenticate(0, &i_AppMasterKey))
        return false;

    // Read the 16 byte secret from the card
    byte u8_FileData[16];
    if (!rfid.ReadFileData(CARD_FILE_ID, 0, 16, u8_FileData))
        return false;

    if (memcmp(u8_FileData, u8_StoreValue, 16) != 0)
        return false;

    return true;
}

// Store the SECRET_PICC_MASTER_KEY on the card
bool ChangePiccMasterKey()
{
    byte u8_KeyVersion;
    if (!AuthenticatePICC(&u8_KeyVersion))
        return false;

    if (u8_KeyVersion != CARD_KEY_VERSION) // empty card
    {
        // Store the secret PICC master key on the card.
        if (!rfid.ChangeKey(0, &gi_PiccMasterKey, NULL))
            return false;

        // A key change always requires a new authentication
        if (!rfid.Authenticate(0, &gi_PiccMasterKey))
            return false;
    }
    return true;
}

// Create the application SECRET_APPLICATION_ID,
// store the dynamic Application master key in the application,
// create a StandardDataFile SECRET_FILE_ID and store the dynamic 16 byte value into that file.
// This function requires previous authentication with PICC master key.
bool StoreDesfireSecret(kUser* pk_User)
{
    if (CARD_APPLICATION_ID == 0x000000 || CARD_KEY_VERSION == 0)
        return false; // severe errors in Secrets.h -> abort
  
    DESFIRE_KEY_TYPE i_AppMasterKey;
    byte u8_StoreValue[16];
    if (!GenerateDesfireSecrets(pk_User, &i_AppMasterKey, u8_StoreValue))
        return false;

    // First delete the application (The current application master key may have changed after changing the user name for that card)
    if (!rfid.DeleteApplicationIfExists(CARD_APPLICATION_ID))
        return false;

    // Create the new application with default settings (we must still have permission to change the application master key later)
    if (!rfid.CreateApplication(CARD_APPLICATION_ID, KS_FACTORY_DEFAULT, 1, i_AppMasterKey.GetKeyType()))
	//if (!rfid.CreateApplication(CARD_APPLICATION_ID, KS_FACTORY_DEFAULT, 1, DF_KEY_2K3DES))
        return false;

    // After this command all the following commands will apply to the application (rather than the PICC)
    if (!rfid.SelectApplication(CARD_APPLICATION_ID))
        return false;

    // Authentication with the application's master key is required
    if (!rfid.Authenticate(0, &DEFAULT_APP_KEY))
	//if (!rfid.Authenticate(0, &rfid.DES2_DEFAULT_KEY))
		return false;

    // Change the master key of the application
    if (!rfid.ChangeKey(0, &i_AppMasterKey, NULL))
        return false;

    // A key change always requires a new authentication with the new key
    if (!rfid.Authenticate(0, &i_AppMasterKey))
        return false;

    // After this command the application's master key and it's settings will be frozen. They cannot be changed anymore.
    // To read or enumerate any content (files) in the application the application master key will be required.
    // Even if someone knows the PICC master key, he will neither be able to read the data in this application nor to change the app master key.
//    if (!rfid.ChangeKeySettings(KS_CHANGE_KEY_FROZEN))
//        return false;

    // --------------------------------------------

    // Create Standard Data File with 16 bytes length
    DESFireFilePermissions k_Permis;
    k_Permis.e_ReadAccess         = AR_KEY0;
    k_Permis.e_WriteAccess        = AR_KEY0;
    k_Permis.e_ReadAndWriteAccess = AR_KEY0;
    k_Permis.e_ChangeAccess       = AR_KEY0;
    if (!rfid.CreateStdDataFile(CARD_FILE_ID, &k_Permis, 16))
        return false;

    // Write the StoreValue into that file
    if (!rfid.WriteFileData(CARD_FILE_ID, 0, 16, u8_StoreValue))
        return false;       
  
    return true;
}


// Reads the card in the RF field.
// In case of a Random ID card reads the real UID of the card (requires PICC authentication)
// ATTENTION: If no card is present, this function returns true. This is not an error. (check that pk_Card->u8_UidLength > 0)
// pk_Card->u8_KeyVersion is > 0 if a random ID card did a valid authentication with SECRET_PICC_MASTER_KEY
// pk_Card->b_PN532_Error is set true if the error comes from the PN532.
bool ReadCard(byte u8_UID[8], kCard* pk_Card)
{
    memset(pk_Card, 0, sizeof(kCard));
  
    if (!rfid.ReadPassiveTargetID(u8_UID, &pk_Card->u8_UidLength, &pk_Card->e_CardType))
    {
        pk_Card->b_PN532_Error = true;
        return false;
    }

    if (pk_Card->e_CardType == CARD_DesRandom) // The card is a Desfire card in random ID mode
    {
        #if USE_DESFIRE
            if (!AuthenticatePICC(&pk_Card->u8_KeyVersion))
			{
				Utils::Print("Authentication feild\r\n");
				return false;
			}
            // replace the random ID with the real UID
            if (!rfid.GetRealCardID(u8_UID))
                return false;

            pk_Card->u8_UidLength = 7; // random ID is only 4 bytes
        #else
            Utils::Print("Cards with random ID are not supported in Classic mode.\r\n");
            return false;    
        #endif
    }
    return true;
}

// Waits for the user to approximate the card to the reader
// Timeout = 30 seconds
// Fills in pk_Card competely, but writes only the UID to pk_User.
bool WaitForCard(kUser* pk_User, kCard* pk_Card)
{
    Utils::Print("Please approximate the card to the reader now!\r\nYou have 30 seconds. Abort with ESC.\r\n");
    uint64_t u64_Start = Utils::GetMillis64();
    
    while (true)
    {
        if (ReadCard(pk_User->ID.u8, pk_Card) && pk_Card->u8_UidLength > 0)
        {
            // Avoid that later the door is opened for this card if the card is a long time in the RF field.
            gu64_LastID = pk_User->ID.u64;

            // All the stuff in this function takes about 2 seconds because the SPI bus speed has been throttled to 10 kHz.
            Utils::Print("Processing... (please do not remove the card)\r\n");
            return true;
        }
      
        if ((Utils::GetMillis64() - u64_Start) > 30000)
        {
            Utils::Print("Timeout waiting for card.\r\n");
            return false;
        }

        if (SerialClass::Read() == 27) // ESCAPE
        {
            Utils::Print("Aborted.\r\n");
            return false;
        }
    }
}

// returns true if the cause of the last error was a Timeout.
// This may happen for Desfire cards when the card is too far away from the reader.
bool IsDesfireTimeout()
{
    #if USE_DESFIRE
        // For more details about this error see comment of GetLastPN532Error()
        if (rfid.GetLastPN532Error() == 0x01) // Timeout
        {
            Utils::Print("A Timeout mostly means that the card is too far away from the reader.\r\n");
            
            // In this special case we make a short pause only because someone tries to open the door 
            // -> don't let him wait unnecessarily.
           // FlashLED(PC15, 200);
            return true;
        }
    #endif
    return false;
}

// If you have already written the master key to a card and want to use the card for another purpose 
// you can restore the master key with this function. Additionally the application SECRET_APPLICATION_ID is deleted.
// If a user has been stored in the EEPROM for this card he will also be deleted.
bool RestoreDesfireCard()
{
    kUser k_User;
    kCard k_Card;  
    if (!WaitForCard(&k_User, &k_Card))
        return false;

    UserManager::DeleteUser(k_User.ID.u64, NULL);    

    if ((k_Card.e_CardType & CARD_Desfire) == 0)
    {
        Utils::Print("The card is not a Desfire card.\r\n");
        return false;
    }

    byte u8_KeyVersion;
    if (!AuthenticatePICC(&u8_KeyVersion))
        return false;

    // If the key version is zero AuthenticatePICC() has already successfully authenticated with the factory default DES key
    if (u8_KeyVersion == 0)
        return true;

    // An error in DeleteApplication must not abort. 
    // The key change below is more important and must always be executed.
    bool b_Success = rfid.DeleteApplicationIfExists(CARD_APPLICATION_ID);
    if (!b_Success)
    {
        // After any error the card demands a new authentication
        if (!rfid.Authenticate(0, &gi_PiccMasterKey))
            return false;
    }
    
    if (!rfid.ChangeKey(0, &rfid.DES2_DEFAULT_KEY, NULL))
        return false;

    // Check if the key change was successfull
    if (!rfid.Authenticate(0, &rfid.DES2_DEFAULT_KEY))
        return false;

    return b_Success;
}

// Waits until the user either hits 'Y' or 'N'
// Timeout = 30 seconds
bool WaitForKeyYesNo()
{
    uint64_t u64_Start = Utils::GetMillis64();
    while (true)
    {
        char c_Char = SerialClass::Read();
        if  (c_Char == 'n' || c_Char == 'N' || (Utils::GetMillis64() - u64_Start) > 30000)
        {
            Utils::Print("Aborted.\r\n");
            return false;
        }
            
        if  (c_Char == 'y' || c_Char == 'Y')
             return true;

        delay(200);
    } 
}

bool MakeRandomCard()
{
    Utils::Print("\r\nATTENTION: Configuring the card to send a random ID cannot be reversed.\r\nThe card will be a random ID card FOREVER!\r\nIf you are really sure what you are doing hit 'Y' otherwise hit 'N'.\r\n\r\n");
    if (!WaitForKeyYesNo())
        return false;
    
    kUser k_User;
    kCard k_Card;  
    if (!WaitForCard(&k_User, &k_Card))
        return false;

    if ((k_Card.e_CardType & CARD_Desfire) == 0)
    {
        Utils::Print("The card is not a Desfire card.\r\n");
        return false;
    }

    byte u8_KeyVersion;
    if (!AuthenticatePICC(&u8_KeyVersion))
        return false;

    return rfid.EnableRandomIDForever();
}

int _tmain(void)
{
    const int DES_SIZE =  8; // One DES block =  8 bytes
    const int AES_SIZE = 16; // One AES block = 16 bytes
    Desfire i_Desfire;

    // ##################################################
    // Testing encryption and decryption
    // ##################################################

    byte u8_Data   [DES_SIZE] = { 'H', 'e', 'l', 'l', 'o', '!', '@', '#' };
    byte u8_Crypt  [DES_SIZE] = {0};
    byte u8_Decrypt[DES_SIZE] = {0};

    if (!i_Desfire.DES3_DEFAULT_KEY.CryptDataBlock(u8_Crypt,   u8_Data,  KEY_ENCIPHER) ||
        !i_Desfire.DES3_DEFAULT_KEY.CryptDataBlock(u8_Decrypt, u8_Crypt, KEY_DECIPHER))
        return 0;

    Utils::Print("8 data bytes:", LF);    
    Utils::PrintHexBuf(u8_Data, DES_SIZE, LF);

    Utils::Print("Encrypted with default DES3 key:", LF);
    Utils::PrintHexBuf(u8_Crypt, DES_SIZE, LF);
	Utils::Print((const char *)u8_Crypt, LF);
	
    Utils::Print("Decrypted with default DES3 key:", LF);
    Utils::PrintHexBuf(u8_Decrypt, DES_SIZE, LF);
	Utils::Print((const char *)u8_Decrypt, LF);
    Utils::Print(LF, LF);

    // ##################################################
    // Testing CMAC calculation
    // 
    // See "Desfire EV1 Communication Examples.htm":
    //
    // ........
    // * SessKey:   90 F7 A2 01 91 03 68 45 EC 63 DE CD 54 4B 99 31 (AES)
    // 
    // *** GetKeyVersion()
    // TX CMAC:  25 7F C5 38 61 8A 94 4A 3A 20 96 7B 6F 31 43 48
    // Sending:  00 00 FF 05 FB <D4 40 01 64 00> 87 00
    // Response: 00 00 FF 0D F3 <D5 41 00 00 10 8A 8F A3 6F 55 CD 21 0D> 5F 00
    // RX CMAC:  8A 8F A3 6F 55 CD 21 0D D8 05 46 58 AC 70 D9 9A
    // Version: 0x10
    // ##################################################

    byte u8_SessKey[AES_SIZE] = { 0x90, 0xF7, 0xA2, 0x01, 0x91, 0x03, 0x68, 0x45, 0xEC, 0x63, 0xDE, 0xCD, 0x54, 0x4B, 0x99, 0x31 };

    AES i_SessionKey;
    if (!i_SessionKey.SetKeyData(u8_SessKey, sizeof(u8_SessKey), 0) ||
        !i_SessionKey.GenerateCmacSubkeys())
        return 0;

    Utils::Print("Session Key:", LF);
    i_SessionKey.PrintKey(LF);

    // Calculate CMAC of send data
    TX_BUFFER(i_TxData, AES_SIZE);
    i_TxData.AppendUint8(DF_INS_GET_KEY_VERSION); // Command     = 0x64
    i_TxData.AppendUint8(0);                      // Parameter 1 = Key 0

    byte u8_Cmac[AES_SIZE];
    if (!i_SessionKey.CalculateCmac(i_TxData, u8_Cmac))
        return 0;

    Utils::Print("TX CMAC of Command GET_KEY_VERSION:", LF);
    Utils::PrintHexBuf(u8_Cmac, sizeof(u8_Cmac), LF);

    // -------------------

    // Calculate CMAC of receive data
    TX_BUFFER(i_RxData, AES_SIZE);
    i_RxData.AppendUint8(0x10);       // Key version
    i_RxData.AppendUint8(ST_Success); // Status 0

    if (!i_SessionKey.CalculateCmac(i_RxData, u8_Cmac))
        return 0;

    Utils::Print("RX CMAC of Response GET_KEY_VERSION:", LF);
    Utils::PrintHexBuf(u8_Cmac, sizeof(u8_Cmac), LF);


    // ##################################################
    // Wait for a keypress before exiting
    // ##################################################

}


#endif // USE_DESFIRE
inline void LED_Blink(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin, uint32_t Delay)
{
	HAL_Delay(Delay);
	HAL_GPIO_WritePin(GPIOx, GPIO_Pin, GPIO_PIN_SET);
	HAL_Delay(Delay);
	HAL_GPIO_WritePin(GPIOx, GPIO_Pin, GPIO_PIN_RESET);
}



/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
	/* USER CODE BEGIN 1 */
	/* USER CODE END 1 */


	/* MCU Configuration--------------------------------------------------------*/

	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	HAL_Init();

	/* USER CODE BEGIN Init */
	/* USER CODE END Init */

	/* Configure the system clock */
	SystemClock_Config();

	/* USER CODE BEGIN SysInit */
	/* USER CODE END SysInit */

	/* Initialize all configured peripherals */
	MX_GPIO_Init();
	MX_SPI1_Init();
	//MX_USB_DEVICE_Init();
	MX_I2C1_Init();
	MX_USART2_UART_Init();
	/* USER CODE BEGIN 2 */
	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	uint8_t uid_buffer[100], uid_length ;
	bool flag_OK = 0;
	//Read = 1, Write = 1 
	typedef enum 
	{
		WRITE,
		READ,
		CHANGE_KEY,
		MIFARE_WRITE,
		MIFARE_READ
	}Phase;
	
	Phase COMMAND;
	
	rfid.begin(); 
	
	LED_Blink(green_port, green_pin, 10);
	do
	{
		while(!HAL_GPIO_ReadPin(Button_Blue_GPIO_Port, Button_Blue_Pin));
		if (rfid.GetFirmwareVersion(&IC, &VersionHi, &VersionLo, &Flags))   
		{
			char Buf[80];
			sprintf(Buf, "Chip: PN5%02X, Firmware version: %d.%d\r\n", IC, VersionHi, VersionLo);
			Utils::Print(Buf);
			sprintf(Buf, "Supports ISO 14443A:%s, ISO 14443B:%s, ISO 18092:%s\r\n", (Flags & 1) ? "Yes" : "No",
																					(Flags & 2) ? "Yes" : "No",
																					(Flags & 4) ? "Yes" : "No");
			Utils::Print(Buf);

			rfid.SetPassiveActivationRetries();
			if(rfid.SamConfig())
			{
				Utils::Print("SAM config done", LF);
				config = 1;
			}
			else
				Utils::Print("SAM config FAILD", LF);
		}
		else
		{
			Utils::Print("FAILD", LF);
		}
	}while(!config);
	
	
	kUser k_User;
	kCard k_Card;
	uint8_t u8_Version ,key_version ,key_number;
	uint32_t AppID[30], IDlist[30];
	uint8_t app_count ,key_count ,file_count ,file_id[32];
	DESFireCardVersion CardDetails;
	DESFireKeySettings KeySettings;
	DESFireFileSettings FileSetting;
	DESFireKeyType KeyType;
	eCardType CardType;
	

	DESFIRE_KEY_TYPE i_AppMasterKey;
	Classic mifare;

	// Create Standard Data File with 16 bytes length
	DESFireFilePermissions k_Permis;
	k_Permis.e_ReadAccess         = AR_KEY0;
	k_Permis.e_WriteAccess        = AR_KEY0;
	k_Permis.e_ReadAndWriteAccess = AR_KEY0;
	k_Permis.e_ChangeAccess       = AR_KEY0;
	byte to_save_mahdi[16]={'m', 'a' ,'h', 'd' ,'i', '-' ,'a', 'm' ,'i' ,'r', 'i' ,'n', 'a' ,'s', 'a' ,'b'};
	byte to_save_mehran[16]={'m', 'e' ,'h', 'r' ,'a', 'n' ,'-' ,'r', 'a' ,'s' ,'t', 'g' ,'a', 'r', 0, 0};

	do{
		
		Utils::Print("Type W to start write process",LF);
		Utils::Print("Type R to start read process",LF);
		Utils::Print("Type C to change master key",LF);
		Utils::Print("Type D to read mifare card block number 4",LF);
		Utils::Print("Type E to Write mifare card block number 4 some credit",LF);
		while(!SerialClass::Available()); // wait for a character
		char char_in = SerialClass::Read ();
		if((char_in == 'W') || (char_in == 'w'))
		{
			COMMAND = WRITE;
			flag_OK = 1;
		}
		else if((char_in == 'R') || (char_in == 'r'))
		{
			COMMAND = READ;
			flag_OK = 1;
		}
		else if ((char_in == 'C') || (char_in == 'c'))
		{
			COMMAND = CHANGE_KEY;
			flag_OK = 1;
		}
		else if ((char_in == 'D') || (char_in == 'd'))
		{
			COMMAND = MIFARE_READ;
			flag_OK = 1;
		}
		else if ((char_in == 'E') || (char_in == 'e'))
		{
			COMMAND = MIFARE_WRITE;
			flag_OK = 1;
		}
		else
		{
			Utils::Print("",LF);
			flag_OK = 0;
		}
	}while(!flag_OK);
	
	
//	HAL_Delay(1000);
	
//	if (HAL_GPIO_ReadPin(Button_Blue_GPIO_Port, Button_Blue_Pin))
//		COMMAND = WRITE;
//	else
//		COMMAND = READ;
	char buff[50];
	sprintf(buff, "Selected process is : %s\r\n", (COMMAND & 1) ? "READ": "WRITE");
	Utils::Print(buff);
	
//	HAL_Delay(1000);
//	Utils::Print("Press the blue button to continue ");
//	
//	while(!HAL_GPIO_ReadPin(Button_Blue_GPIO_Port, Button_Blue_Pin));
//	HAL_Delay(1000);
	while(COMMAND == WRITE)
	{
	/* USER CODE END WHILE */
	/* USER CODE BEGIN 3 */
		
		if (ReadCard(k_User.ID.u8, &k_Card) && k_Card.u8_UidLength > 4)
		{
			LED_Blink(orange_port, orange_pin, Delay_Blink);
			//rfid.FormatCard();
			uint8_t encrypted_data_pre[FileSize] = {0};
			memcpy(encrypted_data_pre, SELECT_USER, FileSize);
			
			Utils::Print("Copping original data\nData is(HEX):",LF);
			Utils::Print((const char *)encrypted_data_pre, LF);

			if (AuthenticatePICC(&k_Card.u8_KeyVersion))
				do
				{
					
					// First delete the application (The current application master key may have changed after changing the user name for that card)
					if (!rfid.DeleteApplicationIfExists(CARD_APPLICATION_ID))
						break;
					
					// Create the new application with default settings (we must still have permission to change the application master key later)
					//if (!rfid.CreateApplication(CARD_APPLICATION_ID, KS_FACTORY_DEFAULT, 5, i_AppMasterKey.GetKeyType()))
					if (!rfid.CreateApplication(CARD_APPLICATION_ID, KS_FACTORY_DEFAULT, 1, DF_KEY_2K3DES))
						break;
					
					// After this command all the following commands will apply to the application (rather than the PICC)
					if (!rfid.SelectApplication(CARD_APPLICATION_ID))
						break;
					
					// Authentication with the application's master key is required
					//if (!rfid.Authenticate(0, &DEFAULT_APP_KEY))
					
					if (!rfid.Authenticate(0, &rfid.DES2_DEFAULT_KEY))
						break;   
					
					if(!rfid.GetKeySettings(&KeySettings,&key_count ,&KeyType))
						break;
					
					if (!rfid.CreateStdDataFile(CARD_FILE_ID, &k_Permis, FileSize))
						break;
					if (!rfid.GetFileSettings(0 ,&FileSetting))
						break;
					// Write the StoreValue into that file
					if (!rfid.WriteFileData(CARD_FILE_ID, 0, FileSize, SELECT_USER))
						break; 
					LED_Blink(green_port, green_pin, Delay_Blink);
					Utils::Print("Write SUCCESSFULLY",LF);
					
				}while(0);
				
				else
				{
					Utils::Print("Write FAILED",LF);
					LED_Blink(red_port, red_pin, Delay_Blink);
				}
			
			break;
		}
		else
		{
			if (IsDesfireTimeout())
			{
				Utils::Print("IsDesfireTimeout", LF);
				// Nothing to do here because IsDesfireTimeout() prints additional error message and blinks the red LED
			}
			else if (k_Card.b_PN532_Error) // Another error from PN532 -> reset the chip
			{
				Utils::Print("Another error from PN532 -> reset the chip", LF);
				
			}
			else if (!(k_Card.u8_UidLength > 4))
			{
				Utils::Print("The card is not a DESFire card", LF);
			}
			else // e.g. Error while authenticating with master key
			{
			   Utils::Print("Error while authenticating with master key", LF);
			}
			
			Utils::Print("> ");
			
			HAL_Delay(200);
			if(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0))
			{
				Utils::Print("Restoring the card...", LF);
				if(RestoreDesfireCard())
					Utils::Print("The card suuccessfully restored", LF);
			}
			break;
		}
		

	}	
	while(COMMAND == READ)
	{
		Utils::Print("Reading the card" ,LF);
		
		if (ReadCard(k_User.ID.u8, &k_Card) && k_Card.u8_UidLength > 4)
		{
			if(AuthenticatePICC(&k_Card.u8_KeyVersion))
				do 
				{
					//if (!rfid.CreateApplication(CARD_APPLICATION_ID, KS_FACTORY_DEFAULT, 5, i_AppMasterKey.GetKeyType()))
					if (!rfid.SelectApplication(CARD_APPLICATION_ID))
						break;
					// Authentication with the application's master key is required
					//if (!rfid.Authenticate(0, &DEFAULT_APP_KEY))
					if (!rfid.Authenticate(0, &rfid.DES2_DEFAULT_KEY))
						break;   
					if (!rfid.GetFileIDs(file_id, &file_count))
						break;
					if (file_count != 1 || file_id[0] != 0)
						Utils::Print("GetFileIDs() failed\r\n");
						
					
					uint8_t read_card_buffer[16];
					if(!rfid.ReadFileData(0 ,0 ,FileSize ,read_card_buffer))
						break;
					
					Utils::Print("Recieved data in string:", LF);
					Utils::Print((const char *)read_card_buffer, LF);
					Utils::Print("Recieved data in HEX:", LF);
					Utils::PrintHexBuf(read_card_buffer,FileSize ,LF);
					sprintf(buff,"File size is: %d", FileSize);
					Utils::Print(buff,LF);
				
					Utils::Print("Read SUCCESSFULLY",LF);	
					LED_Blink(green_port, green_pin, Delay_Blink);
					flag_OK = 1;
						
				}while(0);
			if(!flag_OK)
			{
				Utils::Print("Read FAILED",LF);	
				LED_Blink(red_port, red_pin, Delay_Blink);
				flag_OK = 0;	
			}
		}
		break;
		
	}
	while(COMMAND == CHANGE_KEY)
	{
		Utils::Print("Changing the master key" ,LF);
		 DESFIRE_KEY_TYPE  new_key_class;
		const byte new_key_array [9] = "01234567";
		// If the key is an AES key only the first 16 bytes will be used
		if (!new_key_class.SetKeyData(new_key_array, 8 , CARD_KEY_VERSION))
			break;
		sprintf(buff,"new key is: %s", new_key_array);
		Utils::Print(buff, LF);
		
		if (ReadCard(k_User.ID.u8, &k_Card) && k_Card.u8_UidLength > 4)
		{
			if(AuthenticatePICC(&k_Card.u8_KeyVersion))
				do 
				{
					// Change the master key of the application
					if (!rfid.ChangeKey(0, &new_key_class, NULL))
						break;
	
					// A key change always requires a new authentication with the new key
					if (!rfid.Authenticate(0, &new_key_class))
						break;

					Utils::Print("Changing the master key SUCCESSFULLY",LF);	
					LED_Blink(green_port, green_pin, Delay_Blink);
					flag_OK = 1;
				}while(0);
				
			if(!flag_OK)
			{
				Utils::Print("Changing the master key FAILED",LF);	
				LED_Blink(red_port, red_pin, Delay_Blink);
				flag_OK = 0;	
			}
		}
		break;
	}
			
	while(COMMAND == MIFARE_READ)
	{
		
		if (!mifare.ReadPassiveTargetID(uid_buffer, &uid_length, &CardType))
			break;

		if (uid_length == 0)
		{
			//Utils::Print("No card present.\r\n");
			break;
		}
		else if (uid_length == 7)
		{
			Utils::Print("DESFire card not allowded in this command.\r\n");
		}
		else if (uid_length == 4)
		{
			Utils::Print("mifare classic card detected.\r\n");
		}

		byte readout_data[20];
		
		// Authorizing the first block of a sector is enough (1 sector = 4 blocks)
		const uint8_t K[] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
		if(!mifare.AuthenticateDataBlock(4, 'A', K, uid_buffer, uid_length))
		{
			Utils::Print("block 4 authentication failed",LF);
			break;
		}
		else
		{
			Utils::Print("block 4 authentication successful",LF);
		}
		
		if (mifare.ReadDataBlock(4, readout_data))
        {
            Utils::Print("Data block 4 is(HEX):");
			Utils::PrintHexBuf(readout_data, 16);
		}
		
		break;
	}
	
	while(COMMAND == MIFARE_WRITE)
	{
		if (!mifare.ReadPassiveTargetID(uid_buffer, &uid_length, &CardType))
			break;

		if (uid_length == 0)
		{
			//Utils::Print("No card present.\r\n");
			break;
		}
		else if (uid_length == 7)
		{
			Utils::Print("DESFire card not allowded in this command.\r\n");
		}
		else if (uid_length == 4)
		{
			Utils::Print("mifare classic card detected.\r\n");
		}

		
		// Authorizing the first block of a sector is enough (1 sector = 4 blocks)
		const uint8_t K[] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
		if(!mifare.AuthenticateDataBlock(4, 'A', K, uid_buffer, uid_length))
		{
			Utils::Print("block 4 authentication failed",LF);
			break;
		}
		else
		{
			Utils::Print("block 4 authentication successful",LF);
		}
		
		byte writein_data[20] = {0xFF, 0xA0};
		if(mifare.WriteDataBlock(4, writein_data))
			Utils::Print("Writing successful");
		else
			Utils::Print("Writing Failed");
		
			
		break;
	}
	rfid.PowerDown();
	Utils::Print("Program has terminated. reset the chip", LF);
	/* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage 
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 96;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */
  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */
  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_LSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */
  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(CP_SLC_GPIO_Port, CP_SLC_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15, GPIO_PIN_RESET);

  /*Configure GPIO pin : Button_Blue_Pin */
  GPIO_InitStruct.Pin = Button_Blue_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(Button_Blue_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : CP_SLC_Pin */
  GPIO_InitStruct.Pin = CP_SLC_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(CP_SLC_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : RSTO_Pin IRQ_Pin */
  GPIO_InitStruct.Pin = RSTO_Pin|IRQ_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : PD12 PD13 PD14 PD15 */
  GPIO_InitStruct.Pin = GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* USER CODE END Error_Handler_Debug */
}


#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{ 
  /* USER CODE BEGIN 6 */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
