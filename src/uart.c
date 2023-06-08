/*****************************************
		NanoShell Operating System
		  (C) 2022 iProgramInCpp

            Serial port module
******************************************/
#include <uart.h>
#include <string.h>
#include <vfs.h>

const short g_uart_port_bases[] = { 0x3F8, 0x2F8 };//, 0x3E8, 0x2E8 };

// Setting the baud rate
#define DLAB_ENABLE    (1 << 7)
#define BAUD_DIVISOR   (0x0003)

// Line Control register
#define LCR_5BIT_DATA (0 << 0) // Data Bits - 5
#define LCR_6BIT_DATA (1 << 0) // Data Bits - 6
#define LCR_7BIT_DATA (2 << 0) // Data Bits - 7
#define LCR_8BIT_DATA (3 << 0) // Data Bits - 8
#define LCR_PAR_SPACE (7 << 3) // SPACE Parity.
#define LCR_PAR_NONE  (0 << 3) // No Parity.

//c7 = 11000111

// FIFO Control register
#define FCR_ENABLE (1 << 0)
#define FCR_RFRES  (1 << 1)
#define FCR_XFRES  (1 << 2)
#define FCR_DMASEL (1 << 3)
#define FCR_RXTRIG (3 << 6)

// Interrupt Enable register
#define IER_NOINTS (0)

// Modem Control Register
#define MCR_DTR    (1 << 0)
#define MCR_RTS    (1 << 1)
#define MCR_OUT1   (1 << 2)
#define MCR_OUT2   (1 << 3)
#define MCR_LOOPBK (1 << 4) // loop-back

// Line status register
#define LSR_RBF    (1 << 0)
#define LSR_OE     (1 << 1)
#define LSR_PE     (1 << 2)
#define LSR_FE     (1 << 3)
#define LSR_BREAK  (1 << 4)
#define LSR_THRE   (1 << 5)
#define LSR_TEMT   (1 << 6)
#define LSR_FIFOER (1 << 7)

// Modem status register
#define MSR_DCTS   (1 << 0)
#define MSR_DDSR   (1 << 1)
#define MSR_TERI   (1 << 2)
#define MSR_DDCD   (1 << 3)
#define MSR_CTS    (1 << 4)
#define MSR_DSR    (1 << 5)
#define MSR_RI     (1 << 6)
#define MSR_DCD    (1 << 7)

// Interrupt Enable register
#define IER_ERBFI  (1 << 0) //Enable receive  buffer full  interrupt
#define IER_ETBEI  (1 << 1) //Enable transmit buffer empty interrupt
#define IER_ELSI   (1 << 2) //Enable line status interrupt
#define IER_EDSSI  (1 << 3) //Enable delta status signals interrupt

// Interrupt ID Register
#define IID_PEND   (1 << 0) // An interrupt is pending if this is zero
#define IID_0      (1 << 1)
#define IID_1      (1 << 2)
#define IID_2      (1 << 3)

// Port offsets
#define S_RBR 0x00 // Receive buffer register (read only) same as...
#define S_THR 0x00 // Transmitter holding register (write only)
#define S_IER 0x01 // Interrupt enable register
#define S_IIR 0x02 // Interrupt ident register (read only)...
#define S_FCR 0x02 // FIFO control register (write only)
#define S_LCR 0x03 // Line control register
#define S_MCR 0x04 // Modem control register
#define S_LSR 0x05 // Line status register
#define S_MSR 0x06 // Modem status register

#define S_CHECK_BYTE 0xCA

bool UartIsTransmitEmpty(uint8_t comNum)
{
	short Port = g_uart_port_bases[comNum];
	return((ReadPort(Port + 5) & 0x20) != 0);
}

bool UartIsReceiveEmpty(uint8_t comNum)
{
	short Port = g_uart_port_bases[comNum];
	return((ReadPort(Port + 5) & 0x01) == 0);
}

void UartOnInterrupt(uint8_t com_num)
{
	// PIC EOI
	WritePort (0x20, 0x20);
	WritePort (0xA0, 0x20);
	
	uint8_t status = ReadPort (S_IIR);
	
	//SLogMsg("Got uart interrupt on COM%d. Status: %b", com_num + 1, status);
	
	status &= 0xF;
	
	if (status == 0x4) // Receiver
	{
		while (!UartIsReceiveEmpty(com_num))
			ReadPort (S_RBR);
	}
}

short UartGetPortBase(uint8_t com_num)
{
	return g_uart_port_bases[com_num];
}

static int FsSerialRead(FileNode* pNode, UNUSED uint32_t offset, uint32_t size, void* pBuffer, bool bBlock)
{
	char* pText = (char*)pBuffer;
	for (int i = 0; i < (int)size; i++)
	{
		if (!UartReadSingleChar(pNode->m_inode, pText + i, bBlock))
			return i;
	}
	
	return size;
}

static int FsSerialWrite(FileNode* pNode, UNUSED uint32_t offset, uint32_t size, const void* pBuffer, bool bBlock)
{
	const char* pText = (const char*)pBuffer;
	for (int i = 0; i < (int)size; i++)
	{
		if (!UartWriteSingleChar(pNode->m_inode, *(pText++), bBlock))
			return i;
		
		// Since we are potentially writing to a terminal, a newline will not also go back
		// to the beginning of the line. We need to send them a return to start of line.
		if (*pText == '\n')
		{
			if (!UartWriteSingleChar (pNode->m_inode, '\r', bBlock))
				return i;
		}
	}
	
	return size;
}

void FsUtilAddArbitraryFileNode(const char* pDirPath, const char* pFileName, FileNode* pSrcNode);;

const FileNodeOps g_UartFileOps =
{
	.Read      = FsSerialRead,
	.Write     = FsSerialWrite,
	//.IoControl = FsSerialIoControl,
};

void UartInit(uint8_t com_num)
{
	//COM1 and COM2
	if (com_num >= 2)
	{
		LogMsg("Cannot initialize COM%d, we don't know what it is", com_num + 1);
		return;
	}
	
	short Port = g_uart_port_bases[com_num];
	
	// Disable All Interrupts
	WritePort (Port+1, IER_NOINTS);
	
	// Enable DLAB (Set baud rate divisor)
	WritePort (Port+3, DLAB_ENABLE);
	
	// Set Divisor to 3 -- 38400 baud
	WritePort (Port+0, BAUD_DIVISOR & 0xFF); 
	WritePort (Port+1, BAUD_DIVISOR  >>  8);
	
	// Set the data parity and bit size. 
	WritePort (Port+3, LCR_8BIT_DATA | LCR_PAR_NONE);
	
	// Enable FIFO
	WritePort (Port+2, FCR_RXTRIG | FCR_XFRES | FCR_RFRES | FCR_ENABLE);
	
	// Prepare modem control register
	WritePort (Port+4, MCR_OUT1 | MCR_RTS | MCR_DTR); // IRQs enabled, RTS/DSR set
	
	// set in loopback mode to test the serial chip
	WritePort (Port+4, MCR_LOOPBK | MCR_OUT1 | MCR_OUT2 | MCR_RTS);
	
	// Send a check byte, and check if we get it back
	WritePort (Port+0, S_CHECK_BYTE);
	
	if (ReadPort (Port + 0) != S_CHECK_BYTE)
		// Nope. Return
		return;
	
	// Set this serial port to normal operation
	WritePort (Port+4, MCR_OUT1 | MCR_OUT2 | MCR_RTS | MCR_DTR); // IRQs enabled, OUT#1 and OUT#2 bits, no loop-back
	
	// Enable interrupts again
	WritePort (Port+1, IER_ERBFI | IER_ETBEI);
	
	// Ok, port init has completed, now add it to the file system
	
	char name[] = "ComX";
	name[3] = '1' + com_num;
	
	FileNode node, *pNode = &node;
	memset(pNode, 0, sizeof node);
	
	pNode->m_refCount = NODE_IS_PERMANENT;
	
	pNode->m_perms  = PERM_READ | PERM_WRITE;
	pNode->m_type   = FILE_TYPE_CHAR_DEVICE;
	pNode->m_inode  = com_num;
	pNode->m_length = 0;
	pNode->m_bHasDirCallbacks = false;
	pNode->m_pFileOps = &g_UartFileOps;
	
	FsUtilAddArbitraryFileNode("/Device", name, pNode);
	
	// Send a testing string out
	char pText[] = "NanoShell has initialized COMX successfully.\r\n";
	pText[29] = '1' + com_num;
	
	char* pnText = pText;
	while (*pnText)
	{
		UartWriteSingleChar(com_num, *pnText, true);
		pnText++;
	}
	
}

bool UartWriteSingleChar(uint8_t com_num, char c, bool bBlock)
{
	while (!UartIsTransmitEmpty(com_num))
	{
		if (!bBlock) return false;
		asm("pause");
	}
	
	WritePort(g_uart_port_bases[com_num], c);
	
	return true;
}

bool UartReadSingleChar(uint8_t com_num, char*c, bool bBlock)
{
	while (UartIsReceiveEmpty (com_num))
	{
		if (!bBlock) return false;
		asm("pause");
	}
	
	*c = ReadPort (g_uart_port_bases[com_num]);
	return true;
}
