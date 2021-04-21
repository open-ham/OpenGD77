using Microsoft.Win32.SafeHandles;
using System;
using System.IO;
using System.Runtime.InteropServices;

namespace UsbLibrary
{
	public abstract class HIDDevice : Win32Usb, IDisposable
	{
		private FileStream m_oFile;

		private int m_nInputReportLength;

		private int m_nOutputReportLength;

		protected IntPtr m_hHandle;

		public int OutputReportLength
		{
			get
			{
				return this.m_nOutputReportLength;
			}
		}

		public int InputReportLength
		{
			get
			{
				return this.m_nInputReportLength;
			}
		}

		public event EventHandler OnDeviceRemoved;

		public void Dispose()
		{
			this.Dispose(true);
			GC.SuppressFinalize(this);
		}

		protected virtual void Dispose(bool bDisposing)
		{
			try
			{
				if (bDisposing && this.m_oFile != null)
				{
					this.m_oFile.Close();
					this.m_oFile = null;
				}
				if (this.m_hHandle != IntPtr.Zero)
				{
					Win32Usb.CloseHandle(this.m_hHandle);
				}
			}
			catch (Exception ex)
			{
				Console.WriteLine(ex.ToString());
			}
		}

		private void method_0(string string_0)
		{
			this.m_hHandle = Win32Usb.CreateFile(string_0, 3221225472u, 3u, IntPtr.Zero, 3u, 1073741824u, IntPtr.Zero);
			if (this.m_hHandle != Win32Usb.InvalidHandleValue)
			{
				IntPtr lpData = default(IntPtr);
				if (Win32Usb.HidD_GetPreparsedData(this.m_hHandle, out lpData))
				{
					try
					{
						HidCaps hidCaps = default(HidCaps);
						Win32Usb.HidP_GetCaps(lpData, out hidCaps);
						this.m_nInputReportLength = hidCaps.InputReportByteLength;
						this.m_nOutputReportLength = hidCaps.OutputReportByteLength;
						this.m_oFile = new FileStream(new SafeFileHandle(this.m_hHandle, false), FileAccess.ReadWrite, this.m_nInputReportLength, true);
					}
					catch (Exception ex)
					{
						Console.WriteLine(ex.Message);
						throw GException0.GenerateWithWinError("Failed to get the detailed data from the hid.");
					}
					finally
					{
						Win32Usb.HidD_FreePreparsedData(ref lpData);
					}
					return;
				}
				throw GException0.GenerateWithWinError("GetPreparsedData failed");
			}
			this.m_hHandle = IntPtr.Zero;
			throw GException0.GenerateWithWinError("Failed to create device file");
		}

		protected bool BeginAsyncRead(byte[] data)
		{
			try
			{
				byte[] array = new byte[this.m_nInputReportLength];
				IAsyncResult asyncResult = this.m_oFile.BeginRead(array, 0, this.m_nInputReportLength, null, null);
				asyncResult.AsyncWaitHandle.WaitOne(3000);
				if (asyncResult.IsCompleted)
				{
					this.m_oFile.EndRead(asyncResult);
					Array.Copy(array, 4, data, 0, this.m_nInputReportLength - 4);
					return true;
				}
				return false;
			}
			catch (Exception ex)
			{
				Console.WriteLine(ex.Message);
				return false;
			}
		}

		protected void BeginAsyncRead()
		{
			byte[] array = new byte[this.m_nInputReportLength];
			array[0] = 3;
			this.m_oFile.BeginRead(array, 0, this.m_nInputReportLength, this.ReadCompleted, array);
		}

		protected void ReadCompleted(IAsyncResult iResult)
		{
			byte[] data = (byte[])iResult.AsyncState;
			try
			{
				this.m_oFile.EndRead(iResult);
				try
				{
					InputReport inputReport = this.CreateInputReport();
					inputReport.SetData(data);
					this.HandleDataReceived(inputReport);
				}
				finally
				{
					this.BeginAsyncRead();
				}
			}
			catch (IOException ex)
			{
				Console.WriteLine(ex.Message);
				this.HandleDeviceRemoved();
				if (this.OnDeviceRemoved != null)
				{
					this.OnDeviceRemoved(this, new EventArgs());
				}
				this.Dispose();
			}
		}

		protected void Write(OutputReport oOutRep)
		{
			try
			{
				int bufferLength = oOutRep.BufferLength;
				this.m_oFile.BeginWrite(oOutRep.Buffer, 0, oOutRep.BufferLength, null, null);
			}
			catch (IOException ex)
			{
				Console.WriteLine(ex.ToString());
				throw new GException0("Probaly the device was removed");
			}
			catch (Exception ex2)
			{
				Console.WriteLine(ex2.ToString());
			}
		}

		protected virtual void HandleDataReceived(InputReport oInRep)
		{
		}

		protected virtual void HandleDeviceRemoved()
		{
		}

		private static string smethod_0(IntPtr intptr_0, ref DeviceInterfaceData deviceInterfaceData_0)
		{
			uint nDeviceInterfaceDetailDataSize = 0u;
			if (!Win32Usb.SetupDiGetDeviceInterfaceDetail(intptr_0, ref deviceInterfaceData_0, IntPtr.Zero, 0u, ref nDeviceInterfaceDetailDataSize, IntPtr.Zero))
			{
				DeviceInterfaceDetailData deviceInterfaceDetailData = default(DeviceInterfaceDetailData);
				if (IntPtr.Size == 4)
				{
					deviceInterfaceDetailData.Size = 5;
				}
				else
				{
					deviceInterfaceDetailData.Size = 8;
				}
				if (Win32Usb.SetupDiGetDeviceInterfaceDetail(intptr_0, ref deviceInterfaceData_0, ref deviceInterfaceDetailData, nDeviceInterfaceDetailDataSize, ref nDeviceInterfaceDetailDataSize, IntPtr.Zero))
				{
					return deviceInterfaceDetailData.DevicePath;
				}
			}
			return null;
		}

		public static HIDDevice FindDevice(int nVid, int nPid, Type oType)
		{
			string value = string.Format("vid_{0:x4}&pid_{1:x4}", nVid, nPid);
			Guid hIDGuid = Win32Usb.HIDGuid;
			IntPtr intPtr = Win32Usb.SetupDiGetClassDevs(ref hIDGuid, null, IntPtr.Zero, 18u);
			try
			{
				DeviceInterfaceData deviceInterfaceData = default(DeviceInterfaceData);
				deviceInterfaceData.Size = Marshal.SizeOf(deviceInterfaceData);
				int num = 0;
				while (Win32Usb.SetupDiEnumDeviceInterfaces(intPtr, 0u, ref hIDGuid, (uint)num, ref deviceInterfaceData))
				{
					string text = HIDDevice.smethod_0(intPtr, ref deviceInterfaceData);
					if (text.IndexOf(value) < 0)
					{
						num++;
						continue;
					}
					HIDDevice hIDDevice = (HIDDevice)Activator.CreateInstance(oType);
					hIDDevice.method_0(text);
					return hIDDevice;
				}
			}
			catch (Exception ex)
			{
				throw GException0.GenerateError(ex.ToString());
			}
			finally
			{
				Marshal.GetLastWin32Error();
				Win32Usb.SetupDiDestroyDeviceInfoList(intPtr);
			}
			return null;
		}

		public virtual InputReport CreateInputReport()
		{
			return null;
		}

		protected HIDDevice()
		{
			
			//base._002Ector();
		}
	}
}
