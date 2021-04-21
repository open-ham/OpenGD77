//#define DUMP_USB_INFOS
using System;
using System.Threading;
using LibUsbDotNet;
using LibUsbDotNet.Info;
using System.Collections.ObjectModel;
using LibUsbDotNet.Main;


namespace UsbLibDotNetDevice
{
	public class UsbLibDotNetHIDDevice
	{
		private int m_vendorID;
		private int m_productID;
		private byte m_configID;
		private byte m_interfaceID;
		private ReadEndpointID m_readEndpoint;
		private WriteEndpointID m_writeEndpoint;
		private UsbDevice m_usbDevice;
		private UsbEndpointReader m_usbReader;
		private UsbEndpointWriter m_usbWriter;

		public UsbLibDotNetHIDDevice()
		{
			this.m_vendorID = -1;
			this.m_productID = -1;
			this.m_configID = 255;
			this.m_interfaceID = 255;
			this.m_readEndpoint = ReadEndpointID.Ep01;
			this.m_writeEndpoint = WriteEndpointID.Ep01;

			this.m_usbDevice = null;
			this.m_usbReader = null;
			this.m_usbWriter = null;
		}

		public static UsbLibDotNetHIDDevice FindDevice(int vendorID, int productID, byte configID = 1, byte interfaceID = 0)
		{
			UsbLibDotNetHIDDevice newDev = null;
			UsbDeviceFinder usbFinder = new UsbDeviceFinder(vendorID, productID);
			UsbDevice usbDev = null;
			//Byte configID = 255;
			bool endpointsFound = false;

			try
			{
				// Find and open the usb device.
				usbDev = UsbDevice.OpenUsbDevice(usbFinder);

				// If the device is open and ready
				if (usbDev == null)
				{
					Console.WriteLine("Device Not Found [0x" + vendorID.ToString("x4") + ":0x" + productID.ToString("x4") + "].");
					return null;
				}

				newDev = new UsbLibDotNetHIDDevice();

				for (int iConfig = 0; iConfig < usbDev.Configs.Count; iConfig++)
				{
					UsbConfigInfo configInfo = usbDev.Configs[iConfig];

					if (configID == configInfo.Descriptor.ConfigID)
					{
						ReadOnlyCollection<UsbInterfaceInfo> interfaceList = configInfo.InterfaceInfoList;

						//Console.WriteLine("Config Found: " + configInfo.Descriptor.ConfigID.ToString());
						for (int iInterface = 0; iInterface < interfaceList.Count; iInterface++)
						{
							UsbInterfaceInfo interfaceInfo = interfaceList[iInterface];

							if (interfaceID == interfaceInfo.Descriptor.InterfaceID)
							{
								//Console.WriteLine("Interface Found: " + interfaceInfo.Descriptor.EndpointCount.ToString());
								// We need 2 Endpoints
								if (interfaceInfo.Descriptor.EndpointCount == 2)
								{
									ReadOnlyCollection<UsbEndpointInfo> endpointList = interfaceInfo.EndpointInfoList;

									//Console.WriteLine("Two Endpoints Found");
									for (int iEndpoint = 0; iEndpoint < endpointList.Count; iEndpoint++)
									{
										if (iEndpoint == 0)
										{
											newDev.m_readEndpoint = (ReadEndpointID)endpointList[iEndpoint].Descriptor.EndpointID;
										}
										else
										{
											newDev.m_writeEndpoint = (WriteEndpointID)endpointList[iEndpoint].Descriptor.EndpointID;
										}

										newDev.m_configID = configInfo.Descriptor.ConfigID;
										newDev.m_interfaceID = interfaceInfo.Descriptor.InterfaceID;
									}

									endpointsFound = true;
								}
							}
						}
					}
				}

				if (String.Compare(System.Environment.GetEnvironmentVariable("USBLIBDOTNET_VERBOSE"), "yes", true) == 0)
				{
					Console.WriteLine("*** GD77 USB Device Infos:\n  - " + usbDev.Info.ToString().Replace("\n", "\n  - "));
					for (int iConfig = 0; iConfig < usbDev.Configs.Count; iConfig++)
					{
						UsbConfigInfo configInfo = usbDev.Configs[iConfig];

						Console.WriteLine("   *** ConfigID: " + configInfo.Descriptor.ConfigID);
						Console.WriteLine("   CONFIGURATION INFO: \n     - " + configInfo.ToString().Replace("\n", "\n        - "));

						ReadOnlyCollection<UsbInterfaceInfo> interfaceList = configInfo.InterfaceInfoList;
						for (int iInterface = 0; iInterface < interfaceList.Count; iInterface++)
						{
							UsbInterfaceInfo interfaceInfo = interfaceList[iInterface];
							Console.WriteLine("         *** InterfaceID: " + interfaceInfo.Descriptor.InterfaceID);
							Console.WriteLine("         INTERFACE INFO: \n        - " + interfaceInfo.ToString().Replace("\n", "\n        - "));

							ReadOnlyCollection<UsbEndpointInfo> endpointList = interfaceInfo.EndpointInfoList;
							for (int iEndpoint = 0; iEndpoint < endpointList.Count; iEndpoint++)
							{
								Console.WriteLine("            ENDPOINT LIST: \n           - " + endpointList[iEndpoint].ToString().Replace("\n", "\n           - "));
							}
						}
					}
					Console.WriteLine("***\n");
				}
				if (endpointsFound == false)
				{
					Console.WriteLine("Couldn't find 2 endpoints for interface #" + interfaceID.ToString() + " of configuration #" + configID.ToString());
					return null;
				}
				// If this is a "whole" usb device (libusb-win32, linux libusb)
				// it will have an IUsbDevice interface. If not (WinUSB) the 
				// variable will be null indicating this is an interface of a 
				// device.
				IUsbDevice wholeUsbDevice = usbDev as IUsbDevice;
				if (!ReferenceEquals(wholeUsbDevice, null))
				{
#if DUMP_USB_INFOS
					Console.WriteLine("*** ConfigID: " + newDev.m_configID);
					Console.WriteLine("*** InterfaceID: " + newDev.m_interfaceID);
#endif
					// This is a "whole" USB device. Before it can be used, 
					// the desired configuration and interface must be selected.

					// Select config #1
					wholeUsbDevice.SetConfiguration(newDev.m_configID);

					// Claim interface #0.
					wholeUsbDevice.ClaimInterface(newDev.m_interfaceID);
				}

				// open read endpoint 1.
				newDev.m_usbReader = usbDev.OpenEndpointReader(newDev.m_readEndpoint);
				newDev.m_usbReader.ReadThreadPriority = ThreadPriority.AboveNormal;

				// open write endpoint 2
				newDev.m_usbWriter = usbDev.OpenEndpointWriter(newDev.m_writeEndpoint);
			}
			catch (Exception ex)
			{
				Console.WriteLine("ERROR: " + ex.Message);
				return null;
			}

			newDev.m_usbDevice = usbDev;
			newDev.m_vendorID = vendorID;
			newDev.m_productID = productID;

			return newDev;
		}

		public bool SendAndReceiveData(byte[] cmd, int index, int length, byte[] resp)
		{
			ErrorCode ecWrite;
			ErrorCode ecRead;
			int transferredOut;
			int transferredIn;
			UsbTransfer usbWriteTransfer;
			UsbTransfer usbReadTransfer;
			byte[] readBuffer = new byte[4096];
			byte[] sendBuffer = new byte[4 + cmd.Length];

			// Prepare buffer
			sendBuffer[0] = 1;
			sendBuffer[1] = 0;
			sendBuffer[2] = Convert.ToByte(cmd.Length);
			sendBuffer[3] = Convert.ToByte(cmd.Length >> 8);

			Array.Copy(cmd, index, sendBuffer, 4, Math.Min(length, cmd.Length));

			if (sendBuffer.Length < cmd.Length)
			{
				Console.WriteLine("ERROR: sendBuffer.Length < cmd.Length.");
				return false;
			}

			// Create and submit transfer
			ecRead = this.m_usbReader.SubmitAsyncTransfer(readBuffer, 0, readBuffer.Length, 8000, out usbReadTransfer);
			if (ecRead != ErrorCode.None)
			{
				Console.WriteLine("ERROR: Submit Async Read Failed.");
				return false;
			}
			ecWrite = this.m_usbWriter.SubmitAsyncTransfer(sendBuffer, 0, sendBuffer.Length, 8000, out usbWriteTransfer);
			if (ecWrite != ErrorCode.None)
			{
				Console.WriteLine("ERROR: Submit Async Write Failed.");
				return false;
			}

			WaitHandle.WaitAll(new WaitHandle[] { usbReadTransfer.AsyncWaitHandle/*, usbWriteTransfer.AsyncWaitHandle */}, 300, false);

			if (!usbReadTransfer.IsCompleted)
			{
				Console.Write(" [Zzz]");
				// Give it a bit of time to finish
				Thread.Sleep(5);
			}

			ecWrite = usbWriteTransfer.Wait(out transferredOut);
			ecRead = usbReadTransfer.Wait(out transferredIn);

			usbWriteTransfer.Dispose();
			usbReadTransfer.Dispose();

			if (transferredIn > 0)
			{
				Array.Copy(readBuffer, 4, resp, 0, Math.Min(resp.Length, transferredIn));
			}

			return true;
		}

		public bool SendAndReceiveData(byte[] cmd, byte[] resp)
		{
			return this.SendAndReceiveData(cmd, 0, cmd.Length, resp);
		}

		public static string ByteArrayToString(byte[] ba)
		{
			string hex = BitConverter.ToString(ba);
			return hex.Replace("-", "");
		}

		public void Dispose()
		{
			if (this.m_usbDevice != null)
			{
				if (this.m_usbDevice.IsOpen)
				{
					// If this is a "whole" usb device (libusb-win32, linux libusb-1.0)
					// it exposes an IUsbDevice interface. If not (WinUSB) the 
					// 'wholeUsbDevice' variable will be null indicating this is 
					// an interface of a device; it does not require or support 
					// configuration and interface selection.
					IUsbDevice wholeUsbDevice = this.m_usbDevice as IUsbDevice;
					if (!ReferenceEquals(wholeUsbDevice, null))
					{
						// Release interface #0.
						wholeUsbDevice.ReleaseInterface(this.m_interfaceID);
					}

					this.m_usbDevice.Close();
				}
				this.m_usbDevice = null;
			}

			GC.SuppressFinalize(this);
		}

		~UsbLibDotNetHIDDevice()
		{
			this.Dispose();
		}

	}
}
