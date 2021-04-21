//#define EXTENDED_DEBUG
/*
 * 
 * Copyright (C)2019 Roger Clark. VK3KYY
 * 
 * Encryption sections based on work by Kai DG4KLU
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   1. Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *   3. The name of the author may not be used to endorse or promote products
 *      derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
using System;
using System.Text;
using System.Linq;
#if (LINUX_BUILD)
using UsbLibDotNetDevice;
using System.Collections.Generic;
#else
using System.Collections.Generic;
using System.Threading.Tasks;
using UsbLibrary;
#endif
using System.IO;
using System.Windows.Forms;

namespace GD77_FirmwareLoader
{
	class FirmwareLoader
	{
		private static readonly byte[] responseOK = { 0x41 };

#if (LINUX_BUILD)
		public static UsbLibDotNetHIDDevice _specifiedDevice = null;
		private static readonly string waitPromp = "-\\|/";
        private static int waitPrompIndex = 3;
#else
		private static SpecifiedDevice _specifiedDevice = null;
#endif

		private static readonly int VENDOR_ID = 0x15A2;
		private static readonly int PRODUCT_ID = 0x0073;
		private static FrmProgress _progessForm;

		public enum OutputType
		{
			OutputType_GD77,
			OutputType_GD77S,
			OutputType_DM1801,
			OutputType_RD5R,
			OutputType_UNKNOWN
		}

		class StringAndOutputType
		{
			public byte[] Model { get; set; }
			public OutputType Type { get; set; }
		}

		public static OutputType outputType = OutputType.OutputType_GD77;

		public static String getModelString(OutputType type)
		{
			switch (type)
			{
				case OutputType.OutputType_GD77:
					return "GD-77";
				case OutputType.OutputType_GD77S:
					return "GD-77S";
				case OutputType.OutputType_DM1801:
					return "DM-1801";
				case OutputType.OutputType_RD5R:
					return "RD-5R";
			}

			return "Unknown";
		}

		public static String getModelName()
		{
			return getModelString(outputType);
		}

		public static int UploadFirmware(string fileName, FrmProgress progessForm=null)
		{
			byte[] encodeKey = null;

			_progessForm = progessForm;

			switch (outputType)
			{
				case OutputType.OutputType_GD77:
					encodeKey = new Byte[4] { (0x61 + 0x00), (0x61 + 0x0C), (0x61 + 0x0D), (0x61 + 0x01) };
					Console.WriteLine(" - GD-77 Support");
					break;

				case OutputType.OutputType_GD77S:
					encodeKey = new Byte[4] { (0x47), (0x70), (0x6d), (0x4a) };
					Console.WriteLine(" - GD-77S Support");
					break;

				case OutputType.OutputType_DM1801:
					encodeKey = new Byte[4] { (0x74), (0x21), (0x44), (0x39) };
					Console.WriteLine(" - DM-1801 Support");
					break;

				case OutputType.OutputType_RD5R:
					encodeKey = new Byte[4] { (0x53), (0x36), (0x37), (0x62) };
					Console.WriteLine(" - RD-5R Support");
					break;


				case OutputType.OutputType_UNKNOWN:
					Console.WriteLine("Error. Unknown model type");
					return -99;
			}

#if (LINUX_BUILD)
			_specifiedDevice = UsbLibDotNetHIDDevice.FindDevice(VENDOR_ID, PRODUCT_ID);
#else
			_specifiedDevice = SpecifiedDevice.FindSpecifiedDevice(VENDOR_ID, PRODUCT_ID);
#endif

			if (_specifiedDevice == null)
			{
				Console.WriteLine("Error. Can't connect to the {0}", getModelName()); 
				return -1;
			}

			byte[] fileBuf = File.ReadAllBytes(fileName);
			if (Path.GetExtension(fileName).ToLower() == ".sgl")
			{
				Dictionary<FirmwareLoader.OutputType, byte> firmwareModelTag = new Dictionary<FirmwareLoader.OutputType, byte>();
				byte headerModel = 0x00;

				firmwareModelTag.Add(OutputType.OutputType_GD77, 0x1B);
				firmwareModelTag.Add(OutputType.OutputType_GD77S, 0x70);
				firmwareModelTag.Add(OutputType.OutputType_DM1801, 0x4F);
				firmwareModelTag.Add(OutputType.OutputType_RD5R, 0x5C);         // valid value for DM5R firmware v2.1.7

				// Couls be a SGL file !
				fileBuf = checkForSGLAndReturnEncryptedData(fileBuf, encodeKey, ref headerModel);
				if (fileBuf == null)
				{
					Console.WriteLine("Error. Missing SGL! in .sgl file header");
					_specifiedDevice.Dispose();
					_specifiedDevice = null;
					return -5;
				}

				Console.WriteLine(" - Firmware file confirmed as SGL");

				if (firmwareModelTag[FirmwareLoader.outputType] != headerModel)
				{
					Console.WriteLine("Error. The firmware doesn't match the transceiver model.");
					if (_progessForm != null)
					{
						MessageBox.Show("Error. The firmware doesn't match the transceiver model.", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
					}
					_specifiedDevice.Dispose();
					_specifiedDevice = null;
					return -10;
				}
			}
			else
			{
				Console.WriteLine(" - Firmware file is unencrypted binary");
				fileBuf = encrypt(fileBuf);
			}


			if (fileBuf.Length > 0x7b000)
			{
				Console.WriteLine("Error. Firmware file too large.");
				_specifiedDevice.Dispose();
				_specifiedDevice = null;
				return -2;
			}

			if (sendInitialCommands(encodeKey) == true)
			{
				int respCode = sendFileData(fileBuf);
				if (respCode == 0)
				{
					Console.WriteLine("\n *** Firmware update complete. Please power cycle the {0} ***", getModelName());
					if (_progessForm != null)
					{
						MessageBox.Show(String.Format("Firmware update complete.Please power cycle the {0}.", getModelName()), "Success", MessageBoxButtons.OK, MessageBoxIcon.Information);
					}
				}
				else
				{
					switch (respCode)
					{
						case -1:
							Console.WriteLine("\nError. File to large");
							if (_progessForm != null)
							{
								MessageBox.Show("Error. File to large.", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
							}
							break;
						case -2:
						case -3:
						case -4:
						case -5:
							Console.WriteLine("\nError " + respCode + " While sending data file");
							if (_progessForm != null)
							{
								MessageBox.Show("Error " + respCode + " While sending data file.", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
							}
							break;
					}
					_specifiedDevice.Dispose();
					_specifiedDevice = null;
					return -3;
				}
			}
			else
			{
				Console.WriteLine("\nError while sending initial commands");
				if (_progessForm != null)
				{
					MessageBox.Show("Error while sending initial commands.", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
				}
				_specifiedDevice.Dispose();
				_specifiedDevice = null;
				return -4;
			}

			_specifiedDevice.Dispose();
			_specifiedDevice = null;
			return 0;
		}

		static byte[] sendAndGetResponse(byte[] cmd)
		{
			const int TRANSFER_LENGTH = 38;
			byte[] recBuf = new byte[TRANSFER_LENGTH];

#if (LINUX_BUILD)
			_specifiedDevice.SendAndReceiveData(cmd, recBuf);
#else
			_specifiedDevice.SendData(cmd);
			_specifiedDevice.ReceiveData(recBuf);// Wait for response
#endif

			return recBuf;
		}

		static bool sendAndCheckResponse(byte[] cmd, byte[] resp)
		{
			const int TRANSFER_LENGTH = 38;
			byte[] responsePadded = new byte[TRANSFER_LENGTH];
			byte[] recBuf = new byte[TRANSFER_LENGTH];

			if (resp.Length < TRANSFER_LENGTH)
			{
				Buffer.BlockCopy(resp, 0, responsePadded, 0, resp.Length);
			}

#if (LINUX_BUILD)
			_specifiedDevice.SendAndReceiveData(cmd, recBuf);
#else
			_specifiedDevice.SendData(cmd);
			_specifiedDevice.ReceiveData(recBuf);// Wait for response
#endif

			if (recBuf.SequenceEqual(responsePadded))
			{
				return true;
			}
			else
			{
				Console.WriteLine();
				Console.WriteLine("Error unexpected response from {0}: {1}", getModelName(), BitConverter.ToString(recBuf));
				if (_progessForm != null)
				{
					MessageBox.Show(String.Format("Error unexpected response from {0}: {1}", getModelName(), BitConverter.ToString(recBuf)), "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
				}

				return false;
			}
		}

		static private byte[] createChecksumData(byte[] buf, int startAddress, int endAddress)
		{
			//checksum data starts with a small header, followed by the 32 bit checksum value, least significant byte first
			byte[] checkSumData = { 0x45, 0x4e, 0x44, 0xff, 0xDE, 0xAD, 0xBE, 0xEF };
			int cs = 0;
			for (int i = startAddress; i < endAddress; i++)
			{
				cs = cs + (int)buf[i];
			}

			checkSumData[4] = (byte)(cs % 256);
			checkSumData[5] = (byte)((cs >> 8) % 256);
			checkSumData[6] = (byte)((cs >> 16) % 256);
			checkSumData[7] = (byte)((cs >> 24) % 256);

			return checkSumData;
		}

		static private void updateBlockAddressAndLength(byte[] buf, int address, int length)
		{
			// Length is 16 bits long in bytes 5 and 6
			buf[5] = (byte)((length) % 256);
			buf[4] = (byte)((length >> 8) % 256);

			// Address is 4 bytes long, in the first 4 bytes
			buf[3] = (byte)((address) % 256);
			buf[2] = (byte)((address >> 8) % 256);
			buf[1] = (byte)((address >> 16) % 256);
			buf[0] = (byte)((address >> 24) % 256);
		}

		static private int sendFileData(byte[] fileBuf)
		{
			byte[] dataHeader = new byte[0x20 + 0x06];
			const int BLOCK_LENGTH = 1024;
			int dataTransferSize = 0x20;
			int checksumStartAddress = 0;
			int address = 0;

			if (_progessForm != null)
			{
				_progessForm.SetLabel("Programming data");
			}



			int fileLength = fileBuf.Length;
			int totalBlocks = (fileLength / BLOCK_LENGTH) + 1;

#if EXTENDED_DEBUG
#else
			Console.Write(" - Programming data ");
#if (LINUX_BUILD)
			int cursorLPos = Console.CursorLeft;
			int cursorTPos = Console.CursorTop;
#endif
#endif

			while (address < fileLength)
			{

				if (address % BLOCK_LENGTH == 0)
				{
					checksumStartAddress = address;
				}

				updateBlockAddressAndLength(dataHeader, address, dataTransferSize);


				if (address + dataTransferSize < fileLength)
				{
					Buffer.BlockCopy(fileBuf, address, dataHeader, 6, 32);

					if (sendAndCheckResponse(dataHeader, responseOK) == false)
					{
						Console.WriteLine("Error sending data");
						return -2;
					}

					address = address + dataTransferSize;
					if ((address % 0x400) == 0)
					{
						if (_progessForm != null)
						{
							_progessForm.SetProgressPercentage((address * 100 / BLOCK_LENGTH) / totalBlocks);
						}
#if EXTENDED_DEBUG
						Console.WriteLine("Sent block " + (address / BLOCK_LENGTH) + " of " + totalBlocks);
#else
#if (LINUX_BUILD)
						waitPrompIndex = (waitPrompIndex + 1) % 4;
						Console.SetCursorPosition(cursorLPos, cursorTPos);
						Console.Write(waitPromp[waitPrompIndex]);
#else
						Console.Write(".");
#endif
#endif
						if (sendAndCheckResponse(createChecksumData(fileBuf, checksumStartAddress, address), responseOK) == false)
						{
							Console.WriteLine("Error sending checksum.");
							return -3;
						}
					}
				}
				else
				{
#if EXTENDED_DEBUG
					Console.WriteLine("Sending last block");
#else
#if (LINUX_BUILD)
					waitPrompIndex = (waitPrompIndex + 1) % 4;
					Console.SetCursorPosition(cursorLPos, cursorTPos);
					Console.Write(waitPromp[waitPrompIndex]);
#else
					Console.Write(".");
#endif
#endif

					dataTransferSize = fileLength - address;
					updateBlockAddressAndLength(dataHeader, address, dataTransferSize);
					Buffer.BlockCopy(fileBuf, address, dataHeader, 6, dataTransferSize);

					if (sendAndCheckResponse(dataHeader, responseOK) == false)
					{
						Console.WriteLine("Error sending data.");
						return -4;
					}

					address = address + dataTransferSize;

					if (sendAndCheckResponse(createChecksumData(fileBuf, checksumStartAddress, address), responseOK) == false)
					{
						Console.WriteLine("Error sending checksum.");
						return -5;
					}
				}
			}

#if EXTENDED_DEBUG
#else
#if (LINUX_BUILD)
			Console.SetCursorPosition(cursorLPos, cursorTPos);
			Console.Write(": done.");
#endif
#endif
			return 0;
		}

		static public OutputType probeModel()
		{
			byte[] commandLetterA = new byte[] { 0x41 }; // 'A'
			byte[][] command0 = new byte[][] { new byte[] { 0x44, 0x4f, 0x57, 0x4e, 0x4c, 0x4f, 0x41, 0x44 }, new byte[] { 0x23, 0x55, 0x50, 0x44, 0x41, 0x54, 0x45, 0x3f } }; // DOWNLOAD
			byte[][] command1 = new byte[][] { commandLetterA, responseOK };
			byte[] commandDummy = new byte[] { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
			byte[][][] commandID = { command0, command1 };
			StringAndOutputType[] models = new StringAndOutputType[] {
				   new StringAndOutputType { Model = Encoding.ASCII.GetBytes("DV01"), Type = OutputType.OutputType_GD77   },
				   new StringAndOutputType { Model = Encoding.ASCII.GetBytes("DV02"), Type = OutputType.OutputType_GD77S  },
				   new StringAndOutputType { Model = Encoding.ASCII.GetBytes("DV03"), Type = OutputType.OutputType_DM1801 },
				   new StringAndOutputType { Model = Encoding.ASCII.GetBytes("DV02"), Type = OutputType.OutputType_RD5R } 
				   };
			int commandNumber = 0;
			byte[] resp;


#if (LINUX_BUILD)
			_specifiedDevice = UsbLibDotNetHIDDevice.FindDevice(VENDOR_ID, PRODUCT_ID);
#else
			_specifiedDevice = SpecifiedDevice.FindSpecifiedDevice(VENDOR_ID, PRODUCT_ID);
#endif

			if (_specifiedDevice == null)
			{
				Console.WriteLine("Error. Can't connect the transceiver");
				return OutputType.OutputType_UNKNOWN;
			}

			while (commandNumber < commandID.Length)
			{
				if (sendAndCheckResponse(commandID[commandNumber][0], commandID[commandNumber][1]) == false)
				{
					Console.WriteLine("Error sending command.");
					_specifiedDevice.Dispose();
					_specifiedDevice = null;
					return OutputType.OutputType_UNKNOWN;
				}

				commandNumber = commandNumber + 1;
			}

			resp = sendAndGetResponse(commandDummy);

			if (resp.Length >= 4)
			{
				foreach (StringAndOutputType model in models)
				{
					if (model.Model.SequenceEqual(resp.ToList().GetRange(0, 4).ToArray()))
					{
						_specifiedDevice.Dispose();
						_specifiedDevice = null;
						return model.Type;
					}
				}
			}

			_specifiedDevice.Dispose();
			_specifiedDevice = null;
			return OutputType.OutputType_UNKNOWN;
		}

		static private bool sendInitialCommands(byte[] encodeKey)
		{
			byte[] commandLetterA = new byte[] { 0x41 }; //A
			byte[][] command0 = new byte[][] { new byte[] { 0x44, 0x4f, 0x57, 0x4e, 0x4c, 0x4f, 0x41, 0x44 }, new byte[] { 0x23, 0x55, 0x50, 0x44, 0x41, 0x54, 0x45, 0x3f } }; // DOWNLOAD
			byte[][] command1 = new byte[][] { commandLetterA, responseOK };
			byte[][] command2 = null;
			byte[][] command3 = new byte[][] { new byte[] { 0x46, 0x2d, 0x50, 0x52, 0x4f, 0x47, 0xff, 0xff }, responseOK }; //... F-PROG..
			byte[][] command4 = null;
			byte[][] command5 = null;
			byte[][] command6 = new byte[][] { new byte[] { 0x56, 0x31, 0x2e, 0x30, 0x30, 0x2e, 0x30, 0x31 }, responseOK }; //V1.00.01

			switch (outputType)
			{
				case OutputType.OutputType_GD77:
					command2 = new byte[][] { new byte[] { 0x44, 0x56, 0x30, 0x31, (0x61 + 0x00), (0x61 + 0x0C), (0x61 + 0x0D), (0x61 + 0x01) }, new byte[] { 0x44, 0x56, 0x30, 0x31 } }; //.... last 4 bytes of the command are the offset encoded as letters a - p (hard coded fr
					command4 = new byte[][] { new byte[] { 0x53, 0x47, 0x2d, 0x4d, 0x44, 0x2d, 0x37, 0x36, 0x30, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }, responseOK }; //SG-MD-760
					command5 = new byte[][] { new byte[] { 0x4d, 0x44, 0x2d, 0x37, 0x36, 0x30, 0xff, 0xff }, responseOK }; //MD-760..
					break;

				case OutputType.OutputType_GD77S:
					command2 = new byte[][] { new byte[] { 0x44, 0x56, 0x30, 0x32, 0x47, 0x70, 0x6d, 0x4a }, new byte[] { 0x44, 0x56, 0x30, 0x32 } }; //.... DV02Gpmj (thanks Wireshark)
					command4 = new byte[][] { new byte[] { 0x53, 0x47, 0x2d, 0x4d, 0x44, 0x2d, 0x37, 0x33, 0x30, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }, responseOK }; // SG-MD-730
					command5 = new byte[][] { new byte[] { 0x4d, 0x44, 0x2d, 0x37, 0x33, 0x30, 0xff, 0xff }, responseOK }; // MD-730..
					break;

				case OutputType.OutputType_DM1801:
					command2 = new byte[][] { new byte[] { 0x44, 0x56, 0x30, 0x33, 0x74, 0x21, 0x44, 0x39 }, new byte[] { 0x44, 0x56, 0x30, 0x33 } }; //.... last 4 bytes of the command are the offset encoded as letters a - p (hard coded fr
					command4 = new byte[][] { new byte[] { 0x42, 0x46, 0x2d, 0x44, 0x4d, 0x52, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }, responseOK }; //BF-DMR
					command5 = new byte[][] { new byte[] { 0x31, 0x38, 0x30, 0x31, 0xff, 0xff, 0xff, 0xff }, responseOK }; //1801..
					break;

				case OutputType.OutputType_RD5R:
					command2 = new byte[][] { new byte[] { 0x44, 0x56, 0x30, 0x32, 0x53, 0x36, 0x37, 0x62 }, new byte[] { 0x44, 0x56, 0x30, 0x32 } };
					command4 = new byte[][] { new byte[] { 0x42, 0x46, 0x2D, 0x35, 0x52, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }, responseOK }; //RD-5R
					command5 = new byte[][] { new byte[] { 0x42, 0x46, 0x2D, 0x35, 0x52, 0xff, 0xff, 0xff }, responseOK }; //RD-5R..
					break;
			}

			byte[][] commandErase = new byte[][] { new byte[] { 0x46, 0x2d, 0x45, 0x52, 0x41, 0x53, 0x45, 0xff }, responseOK }; //F-ERASE
			byte[][] commandPostErase = new byte[][] { commandLetterA, responseOK };
			byte[][] commandProgram = { new byte[] { 0x50, 0x52, 0x4f, 0x47, 0x52, 0x41, 0x4d, 0xf }, responseOK };//PROGRAM
			byte[][][] commands = { command0, command1, command2, command3, command4, command5, command6, commandErase, commandPostErase, commandProgram };
#if (LINUX_BUILD)
			bool firstCommand = true;
			string[] commandNames = {"Sending Download command", "Sending ACK", "Sending encryption key", "Sending F-PROG command", "Sending radio modem number", 
				"Sending radio modem number 2", "Sending version", "Sending erase command", "Send post erase command", "Sending Program command"};
#endif
			int commandNumber = 0;

			Buffer.BlockCopy(encodeKey, 0, command2[0], 4, 4);

			// Send the commands which the GD-77 expects before the start of the data
			while (commandNumber < commands.Length)
			{
				if (_progessForm != null)
				{
#if (LINUX_BUILD)
					_progessForm.SetLabel(commandNames[commandNumber]);
#else
					_progessForm.SetLabel("Sending command " + commandNumber);
#endif
				}

#if EXTENDED_DEBUG
#if (LINUX_BUILD)
				Console.WriteLine("Sending command " + commandNames[commandNumber] + " [ " + commandNumber + " ]");
#else
				Console.WriteLine("Sending command " + commandNumber);
#endif
#else
#if (LINUX_BUILD)
				Console.Write(String.Format("{0} - {1}", (firstCommand ? "" : "\n"), commandNames[commandNumber]));
				if (firstCommand)
				{
					firstCommand = false;
				}
#else
				Console.Write(".");
#endif
#endif

				if (sendAndCheckResponse(commands[commandNumber][0], commands[commandNumber][1]) == false)
				{
					Console.WriteLine("Error sending command.");
					return false;
				}
				commandNumber = commandNumber + 1;
			}

#if EXTENDED_DEBUG
#else
			Console.WriteLine();
#endif
			return true;
		}

		static byte[] encrypt(byte[] unencrypted)
		{
			int shift = 0;
			byte[] encrypted = new byte[unencrypted.Length];
			int data;

			switch (outputType)
			{
				case OutputType.OutputType_GD77:
					shift = 0x0807;
					break;

				case OutputType.OutputType_GD77S:
					shift = 0x2a8e;
					break;

				case OutputType.OutputType_DM1801:
					shift = 0x2C7C;
					break;
				case OutputType.OutputType_RD5R:
					shift = 0x306E;
					break;
					
			}

			byte[] encryptionTable = new byte[32768];
			int len = unencrypted.Length;
			for (int address = 0; address < len; address++)
			{
				data = unencrypted[address] ^ DataTable.EncryptionTable[shift++];

				data = ~(((data >> 3) & 0x1F) | ((data << 5) & 0xE0)); // 0x1F is 0b00011111   0xE0 is 0b11100000

				encrypted[address] = (byte)data;

				if (shift >= 0x7fff)
				{
					shift = 0;
				}
			}
			return encrypted;
		}

		static byte[] checkForSGLAndReturnEncryptedData(byte[] fileBuf, byte[] encodeKey, ref byte headerModel)
		{
			byte[] header_tag = new byte[] { (byte)'S', (byte)'G', (byte)'L', (byte)'!' };

			// read header tag
			byte[] buf_in_4 = new byte[4];

			Buffer.BlockCopy(fileBuf, 0, buf_in_4, 0, buf_in_4.Length);

			headerModel = Buffer.GetByte(fileBuf, 11);

			if (buf_in_4.SequenceEqual(header_tag))
			{
				// read and decode offset and xor tag
				//stream_in.Seek(0x000C, SeekOrigin.Begin);
				//stream_in.Read(buf_in_4, 0, buf_in_4.Length);
				Buffer.BlockCopy(fileBuf, 0x000C, buf_in_4, 0, buf_in_4.Length);
				for (int i = 0; i < buf_in_4.Length; i++)
				{
					buf_in_4[i] = (byte)(buf_in_4[i] ^ header_tag[i]);
				}
				int offset = buf_in_4[0] + 256 * buf_in_4[1];
				byte[] xor_data = new byte[] { buf_in_4[2], buf_in_4[3] };

				// read and decode part of the header
				byte[] buf_in_512 = new byte[512];
				//stream_in.Seek(offset + 0x0006, SeekOrigin.Begin);
				//stream_in.Read(buf_in_512, 0, buf_in_512.Length);
				Buffer.BlockCopy(fileBuf, offset + 0x0006, buf_in_512, 0, buf_in_512.Length);
				int xor_idx = 0;
				for (int i = 0; i < buf_in_512.Length; i++)
				{
					buf_in_512[i] = (byte)(buf_in_512[i] ^ xor_data[xor_idx]);
					xor_idx++;
					if (xor_idx == 2)
					{
						xor_idx = 0;
					}
				}

				// dump decoded part of the header
				/*
				Console.WriteLine(String.Format("Offset  : {0:X4}", offset));
				Console.WriteLine(String.Format("XOR-Data: {0:X2}{1:X2}", xor_data[0], xor_data[1]));
				int pos = 0;
				int idx = 0;
				string line1 = "";
				string line2 = "";
				for (int i = 0; i < buf_in_512.Length; i++)
				{
					if (line1 == "")
					{
						line1 = String.Format("{0:X6}: ", i);
					}
					line1 = line1 + String.Format(" {0:X2}", buf_in_512[idx]);
					if ((buf_in_512[idx] >= 0x20) && (buf_in_512[idx] < 0x7f))
					{
						line2 = line2 + (char)buf_in_512[idx];
					}
					else
					{
						line2 = line2 + ".";
					}
					idx++;
					pos++;

					if (pos == 16)
					{
						Console.WriteLine(line1 + " " + line2);
						line1 = "";
						line2 = "";
						pos = 0;
					}
				}
				*/

				// extract encoding key
				
				byte key1 = (byte)(buf_in_512[0x005D] - 'a');
				byte key2 = (byte)(buf_in_512[0x005E] - 'a');
				byte key3 = (byte)(buf_in_512[0x005F] - 'a');
				byte key4 = (byte)(buf_in_512[0x0060] - 'a');
				int encoding_key = (key1 << 12) + (key2 << 8) + (key3 << 4) + key4;
				
				Buffer.BlockCopy(buf_in_512, 0x005D, encodeKey, 0, 4);


				// extract length
				byte length1 = (byte)buf_in_512[0x0000];
				byte length2 = (byte)buf_in_512[0x0001];
				byte length3 = (byte)buf_in_512[0x0002];
				byte length4 = (byte)buf_in_512[0x0003];
				int length = (length4 << 24) + (length3 << 16) + (length2 << 8) + length1;

				// extract encoded raw firmware
				/*FileStream stream_out = new FileStream(args[0] + "_" + String.Format("{0:X4}", encoding_key) + "_" + String.Format("{0:X6}", length) + ".raw", FileMode.Create, FileAccess.Write, FileShare.ReadWrite);
				stream_in.Seek(stream_in.Length - length, SeekOrigin.Begin);
				int c;
				while ((c = stream_in.ReadByte())>=0)
				{
					stream_out.WriteByte((byte)c);
				}*/

				byte[] retBuf = new byte[length];
				Buffer.BlockCopy(fileBuf, fileBuf.Length - length, retBuf, 0, retBuf.Length);
				return retBuf;
			}
			else
			{
				Console.WriteLine("ERROR: SGL! header missing.");
				return null;
			}
		}
	
	}
}
