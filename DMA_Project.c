
#ifdef __USE_CMSIS
#include "LPC17xx.h"
#endif

#include "main.h"
#include <cr_section_macros.h>
#include "md5.h"
#include "iap_driver.h"
#include "payload_generator.h"
#include "leds.h"
#include "lpc_types.h"
#include "lpc17xx_gpdma.h"
//#include "gpio_17xx_40xx.h"
//#include "board.h"


#define LED0_GPIO_PORT_NUM                      0
#define LED0_GPIO_BIT_NUM                       22
#define true	1
#define false	0
#define DMA_SIZE		(PAYLOAD_BLOCK_SIZE)


int False_Flag = LOW;


uint32_t DMADest_Buffer[DMA_SIZE] = {0};
uint8_t  DMAByteArray[PAYLOAD_BLOCK_SIZE];
uint8_t EEPROM_HashArray[MD5_HASH_SIZE_BYTES];
int CompareValue;


// Terminal Counter flag for Channel 0
__IO uint32_t Channel0_TC;

// Error Counter flag for Channel 0
__IO uint32_t Channel0_Err;

		/*DMA Interrupt service routine*/
void DMA_IRQHandler (void)
{
	// check GPDMA interrupt on channel 0
	if (GPDMA_IntGetStatus(GPDMA_STAT_INT, 0))
	{ //check interrupt status on channel 0
		// Check counter terminal status
		if(GPDMA_IntGetStatus(GPDMA_STAT_INTTC, 0))
		{
			// Clear terminate counter Interrupt pending
			GPDMA_ClearIntPending (GPDMA_STATCLR_INTTC, 0);
				Channel0_TC++;
		}
		if (GPDMA_IntGetStatus(GPDMA_STAT_INTERR, 0))
		{
			// Clear error counter Interrupt pending
			GPDMA_ClearIntPending (GPDMA_STATCLR_INTERR, 0);
			Channel0_Err++;
		}
	}
}

int main(void) {
    e_iap_status iap_status;
    int i,j;
    /* Fill the flash with payload and hash data */
    iap_status = (e_iap_status) generator_init();

    //Board_LED_Init();

    if (iap_status != CMD_SUCCESS)
        while(1);   // Error !!!

    led2_init();		//Initialize the led
    led2_off();			//Turn the led off at the start

    NVIC_DisableIRQ(DMA_IRQn);	//DisableDMA interrupt
        /* preemption = 1, sub-priority = 1 */
    NVIC_SetPriority(DMA_IRQn, ((0x01<<3)|0x01));	//Sets DMA interrupt periority

        /* Initialize GPDMA controller */
    GPDMA_Init();

    		/* Start Of checking the hashes generated in the 4k EEPROM Sector*/
    for(i=FLASH_USER_PAYLOAD_START_SECTOR; i< FLASH_USER_PAYLOAD_START_SECTOR+FLASH_USER_SECTORS_4K-1; i++)		//Loop for checking all the 4k sectors
    {
    	DMA_ReadData( sector_start_address[i],  DMA_SIZE);	//Read the first payload of the 4k sector
    	DMA_Wait_Transmission();							//Wait for the first payload to be transmitted

    	for(j=1; j<PAYLOAD_BLOCK_PIECES; j++)				//Loop for checking every Payload in one 4k Sector
    	{
    		 CopyArrayTo_SmallerArray(DMAByteArray, DMADest_Buffer, DMA_SIZE);						//Copy data from the distination buffer to a smaller buffer
    		 DMA_ReadData( sector_start_address[i] + (j*PAYLOAD_BLOCK_SIZE),  DMA_SIZE);			//read the data of the next payload and continue programming the current payload to save time
    		 CopyCharArray_ToCharArray(DMAByteArray,EEPROM_HashArray,MD5_HASH_SIZE_BYTES);			//Save the current payload Hash value in EEPROM_HashArray to be compared with the calculated hash value
    		 Hash_Calculate( DMAByteArray,  PAYLOAD_SIZE_BYTES);									//Calculate the Hash of the current payload and save it in the same array

    		 CompareValue = CompareTwoArrays(DMAByteArray, EEPROM_HashArray, MD5_HASH_SIZE_BYTES );	//Compare the calculated hash value with the readed hash value from the current sector

    		 if(CompareValue != EqualArray)						//If the compared hash values are not equal rise the False_Flag
    			 False_Flag = HIGH;

    		 DMA_Wait_Transmission();				//Wait for the transmission of the next Payload using the DMA
    	}

    	CopyArrayTo_SmallerArray(DMAByteArray, DMADest_Buffer, DMA_SIZE);						//Copy data from the last Payload of the current 4k Sector from the distination buffer to a smaller buffer
    	CopyCharArray_ToCharArray(DMAByteArray,EEPROM_HashArray,MD5_HASH_SIZE_BYTES);			//Save the Hash value of the last payload of the current 4k sector to be compared
    	Hash_Calculate( DMAByteArray,  PAYLOAD_SIZE_BYTES);										//Calculate the Hash and save it in the same array
    	CompareValue = CompareTwoArrays(DMAByteArray, EEPROM_HashArray, MD5_HASH_SIZE_BYTES );	//Compare the calculated hash value with the readed hash value from the current sector

    	if(CompareValue != EqualArray)						//If the compared hash values are not equal rise the False_Flag
    	     False_Flag = HIGH;
    }
			/* End Of checking the hashes generated in the 4k EEPROM Sector*/


			/* Start Of checking the hashes generated in the 32k EEPROM Sector*/
    for(i=FLASH_SECTOR_16; i< FLASH_SECTOR_16+FLASH_USER_SECTORS_32K-1; i++)		//Loop for checking all the 32k sectors
    {
    	DMA_ReadData( sector_start_address[i],  DMA_SIZE);	//Read the first payload of the 32k sector
    	DMA_Wait_Transmission();							//Wait for the first payload to be transmitted

    	for(j=1; j<PAYLOAD_BLOCK_PIECES_32K; j++)
    	{
    		 CopyArrayTo_SmallerArray(DMAByteArray, DMADest_Buffer, DMA_SIZE);						//Copy data from the distination buffer to a smaller buffer
    		 DMA_ReadData( sector_start_address[i] + (j*PAYLOAD_BLOCK_SIZE),  DMA_SIZE);			//read the data of the next payload and continue programming the current payload to save time
    		 CopyCharArray_ToCharArray(DMAByteArray,EEPROM_HashArray,MD5_HASH_SIZE_BYTES);			//Save the current payload Hash value in EEPROM_HashArray to be compared with the calculated hash value
    		 Hash_Calculate( DMAByteArray,  PAYLOAD_SIZE_BYTES);									//Calculate the Hash of the current payload and save it in the same array

    		 CompareValue = CompareTwoArrays(DMAByteArray, EEPROM_HashArray, MD5_HASH_SIZE_BYTES );	//Compare the calculated hash value with the readed hash value from the current sector

    		 if(CompareValue != EqualArray)						//If the compared hash values are not equal rise the False_Flag
    			 False_Flag = HIGH;

    		 DMA_Wait_Transmission();				//Wait for the transmission of the next Payload using the DMA
    	}

    	CopyArrayTo_SmallerArray(DMAByteArray, DMADest_Buffer, DMA_SIZE);						//Copy data from the last Payload of the current 4k Sector from the distination buffer to a smaller buffer
    	CopyCharArray_ToCharArray(DMAByteArray,EEPROM_HashArray,MD5_HASH_SIZE_BYTES);			//Save the Hash value of the last payload of the current 4k sector to be compared
    	Hash_Calculate( DMAByteArray,  PAYLOAD_SIZE_BYTES);										//Calculate the Hash and save it in the same array
    	CompareValue = CompareTwoArrays(DMAByteArray, EEPROM_HashArray, MD5_HASH_SIZE_BYTES );	//Compare the calculated hash value with the readed hash value from the current sector

    	if(CompareValue != EqualArray)						//If the compared hash values are not equal rise the False_Flag
    	     False_Flag = HIGH;
    }
    		/* End Of checking the hashes generated in the 32k EEPROM Sector*/

    while(1)
    {


    	if(False_Flag)		//Flag represents that there is difference between generated hash and saved hash of the data
		{					//Flashing the led if generated hash is different

    		for(i=0;i<6000000;i++){}	//delay
    			led2_invert();
		}
    	else
    		led2_on();			//if generated hash equals Readed hash just turn the led on

    }
    return 0 ;
}

void GPDMA_ChannelInit(uint32_t Source, int Size)
{
	GPDMA_Channel_CFG_Type GPDMACfg;
	// Setup GPDMA channel --------------------------------
	// channel 0
	GPDMACfg.ChannelNum = 0;
	// Source memory
	GPDMACfg.SrcMemAddr = (uint32_t)Source;
	// Destination memory
	GPDMACfg.DstMemAddr = (uint32_t)DMADest_Buffer;
	// Transfer size
	GPDMACfg.TransferSize = Size;
	// Transfer width
	GPDMACfg.TransferWidth = GPDMA_WIDTH_WORD;
	// Transfer type
	GPDMACfg.TransferType = GPDMA_TRANSFERTYPE_M2M;
	// Source connection - unused
	GPDMACfg.SrcConn = 0;
	// Destination connection - unused
	GPDMACfg.DstConn = 0;
	// Linker List Item - unused
	GPDMACfg.DMALLI = 0;
	// Setup channel with given parameter
	GPDMA_Setup(&GPDMACfg);
}

void DMA_ReadData(uint32_t Source, int Size)
{

    GPDMA_ChannelInit(Source , Size );

    	/* Reset terminal counter */
    Channel0_TC = 0;
    	/* Reset Error counter */
    Channel0_Err = 0;
    	// Enable GPDMA channel 0
    GPDMA_ChannelCmd(0, ENABLE);

    	/* Enable GPDMA interrupt */
    NVIC_EnableIRQ(DMA_IRQn);
}

void DMA_Wait_Transmission(void)
{
	while ((Channel0_TC == 0) && (Channel0_Err == 0));	//Wait for the DMA transmission
}

void CopyArrayTo_SmallerArray(uint8_t *ByteArray, uint32_t *FourByte_Array, int FourByteArraySize)
{
	int j;
	for(j=0; j<FourByteArraySize; j++)
	{
		ByteArray[j*4] = FourByte_Array[j] & 0xff;				//Save the first byte of a 4 byte integer
		ByteArray[(j*4)+1] = (FourByte_Array[j]>>8) & 0xff;		//Save the second byte of a 4 byte integer
		ByteArray[(j*4)+2] = (FourByte_Array[j]>>16) & 0xff;	//Save the third byte of a 4 byte integer
		ByteArray[(j*4)+3] = (FourByte_Array[j]>>24) & 0xff;	//Save the Fourth byte of a 4 byte integer
	}
}

void CopyCharArray_ToCharArray(uint8_t *Arr1, uint8_t *Arr2, int Size)		//copy Array 1 in Array 2
{
	int i ;
	for(i=0;i<Size;i++)
		Arr2[i] = Arr1[i];	//Save array 1 in array 2
}

void Hash_Calculate(uint8_t* hash_destination, uint32_t data_size)
{
    MD5_CTX ctx;

    /* Initialize the MD5 context */
    MD5_Init(&ctx);

    /* Calculate MD5 hash with iterative calculation */
    MD5_Update(&ctx, &hash_destination[MD5_HASH_SIZE_BYTES], data_size);

    /* Place the hash in the payload block */
    MD5_Final(hash_destination, &ctx);
}

uint8_t CompareTwoArrays(uint8_t *Arr1 , uint8_t *Arr2 , int Size)
{
	int i ;
	for(i=0; i<Size;i++)
	{
		if(Arr1[i] != Arr2[i])
			return DifferentArray;		//if there is one byte defference return Different array
	}
	return EqualArray;
}


