using Microsoft.Win32.SafeHandles;
using System;
using System.Runtime.InteropServices;

namespace UsbLibrary
{
	[Serializable]
	public class Win32Usb
	{
		[StructLayout(LayoutKind.Sequential, Pack = 1)]
		protected struct Overlapped
		{
			public uint Internal;

			public uint InternalHigh;

			public uint Offset;

			public uint OffsetHigh;

			public IntPtr Event;
		}

		[StructLayout(LayoutKind.Sequential, Pack = 1)]
		protected struct DeviceInterfaceData
		{
			public int Size;

			public Guid InterfaceClassGuid;

			public int Flags;

			public IntPtr Reserved;
		}

		[StructLayout(LayoutKind.Sequential, Pack = 1)]
		protected struct HidCaps
		{
			public short Usage;

			public short UsagePage;

			public short InputReportByteLength;

			public short OutputReportByteLength;

			public short FeatureReportByteLength;

			[MarshalAs(UnmanagedType.ByValArray, SizeConst = 17)]
			public short[] Reserved;

			public short NumberLinkCollectionNodes;

			public short NumberInputButtonCaps;

			public short NumberInputValueCaps;

			public short NumberInputDataIndices;

			public short NumberOutputButtonCaps;

			public short NumberOutputValueCaps;

			public short NumberOutputDataIndices;

			public short NumberFeatureButtonCaps;

			public short NumberFeatureValueCaps;

			public short NumberFeatureDataIndices;
		}

		[StructLayout(LayoutKind.Sequential, Pack = 1)]
		public struct DeviceInterfaceDetailData
		{
			public int Size;

			[MarshalAs(UnmanagedType.ByValTStr, SizeConst = 256)]
			public string DevicePath;
		}

		[StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode, Pack = 1)]
		public class DeviceBroadcastInterface
		{
			public int Size;

			public int DeviceType;

			public int Reserved;

			public Guid ClassGuid;

			[MarshalAs(UnmanagedType.ByValTStr, SizeConst = 256)]
			public string Name;

			public DeviceBroadcastInterface()
			{
				
			}
		}

		public const int WM_DEVICECHANGE = 537;

		public const int DEVICE_ARRIVAL = 32768;

		public const int DEVICE_REMOVECOMPLETE = 32772;

		protected const int DIGCF_PRESENT = 2;

		protected const int DIGCF_DEVICEINTERFACE = 16;

		protected const int DEVTYP_DEVICEINTERFACE = 5;

		protected const int DEVICE_NOTIFY_WINDOW_HANDLE = 0;

		protected const uint PURGE_TXABORT = 1u;

		protected const uint PURGE_RXABORT = 2u;

		protected const uint PURGE_TXCLEAR = 4u;

		protected const uint PURGE_RXCLEAR = 8u;

		protected const uint GENERIC_READ = 2147483648u;

		protected const uint GENERIC_WRITE = 1073741824u;

		protected const uint FILE_SHARE_READ = 1u;

		protected const uint FILE_SHARE_WRITE = 2u;

		protected const uint FILE_FLAG_OVERLAPPED = 1073741824u;

		protected const uint OPEN_EXISTING = 3u;

		protected const uint ERROR_NO_MORE_ITEMS = 259u;

		protected const uint ERROR_IO_PENDING = 997u;

		protected const uint ERROR_INVALID_USER_BUFFER = 1784u;

		protected const uint INFINITE = 4294967295u;

		public static IntPtr NullHandle;

		protected static IntPtr InvalidHandleValue;

		public static Guid HIDGuid
		{
			get
			{
				Guid result = default(Guid);
				Win32Usb.HidD_GetHidGuid(out result);
				return result;
			}
		}

		[DllImport("hid.dll", SetLastError = true)]
		protected static extern void HidD_GetHidGuid(out Guid gHid);

		[DllImport("setupapi.dll", SetLastError = true)]
		protected static extern IntPtr SetupDiGetClassDevs(ref Guid gClass, [MarshalAs(UnmanagedType.LPStr)] string strEnumerator, IntPtr hParent, uint nFlags);

		[DllImport("setupapi.dll", SetLastError = true)]
		protected static extern int SetupDiDestroyDeviceInfoList(IntPtr lpInfoSet);

		[DllImport("setupapi.dll", SetLastError = true)]
		protected static extern bool SetupDiEnumDeviceInterfaces(IntPtr lpDeviceInfoSet, uint nDeviceInfoData, ref Guid gClass, uint nIndex, ref DeviceInterfaceData oInterfaceData);

		[DllImport("setupapi.dll", SetLastError = true)]
		protected static extern bool SetupDiGetDeviceInterfaceDetail(IntPtr lpDeviceInfoSet, ref DeviceInterfaceData oInterfaceData, IntPtr lpDeviceInterfaceDetailData, uint nDeviceInterfaceDetailDataSize, ref uint nRequiredSize, IntPtr lpDeviceInfoData);

		[DllImport("setupapi.dll", SetLastError = true)]
		protected static extern bool SetupDiGetDeviceInterfaceDetail(IntPtr lpDeviceInfoSet, ref DeviceInterfaceData oInterfaceData, ref DeviceInterfaceDetailData oDetailData, uint nDeviceInterfaceDetailDataSize, ref uint nRequiredSize, IntPtr lpDeviceInfoData);

		[DllImport("user32.dll", SetLastError = true)]
		protected static extern IntPtr RegisterDeviceNotification(IntPtr hwnd, DeviceBroadcastInterface oInterface, uint nFlags);

		[DllImport("user32.dll", SetLastError = true)]
		protected static extern bool UnregisterDeviceNotification(IntPtr hHandle);

		[DllImport("hid.dll", SetLastError = true)]
		protected static extern bool HidD_GetPreparsedData(IntPtr hFile, out IntPtr lpData);

		[DllImport("hid.dll", SetLastError = true)]
		protected static extern bool HidD_FreePreparsedData(ref IntPtr pData);

		[DllImport("hid.dll", SetLastError = true)]
		protected static extern int HidP_GetCaps(IntPtr lpData, out HidCaps oCaps);

		[DllImport("hid.dll", SetLastError = true)]
		protected static extern bool HidD_GetFeature(IntPtr lpData, out byte lpReportBuffer, uint uBufferLength);

		[DllImport("hid.dll", SetLastError = true)]
		protected static extern bool HidD_SetFeature(SafeFileHandle hFile, IntPtr Buffer, uint uBufferLength);

		[DllImport("hid.dll", SetLastError = true)]
		protected static extern bool HidD_SetNumInputBuffers(IntPtr hFile, uint nBuffers);

		[DllImport("hid.dll", SetLastError = true)]
		protected static extern bool HidD_GetIndexedString(SafeFileHandle hFile, uint uStringIndex, IntPtr Buffer, uint uBufferLength);

		[DllImport("hid.dll", SetLastError = true)]
		protected static extern bool HidD_GetInputReport(IntPtr hFile, IntPtr Buffer, uint uBufferLength);

		[DllImport("hid.dll", CallingConvention = CallingConvention.StdCall, SetLastError = true)]
		internal static extern bool HidD_GetProductString(IntPtr intptr_0, IntPtr intptr_1, uint uint_0);

		[DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
		protected static extern IntPtr CreateFile(string strName, uint nAccess, uint nShareMode, IntPtr lpSecurity, uint nCreationFlags, uint nAttributes, IntPtr lpTemplate);

		[DllImport("kernel32.dll", SetLastError = true)]
		protected static extern int CloseHandle(IntPtr hFile);

		public static IntPtr RegisterForUsbEvents(IntPtr hWnd, Guid gClass)
		{
			DeviceBroadcastInterface deviceBroadcastInterface = new DeviceBroadcastInterface();
			deviceBroadcastInterface.Size = Marshal.SizeOf(deviceBroadcastInterface);
			deviceBroadcastInterface.ClassGuid = gClass;
			deviceBroadcastInterface.DeviceType = 5;
			deviceBroadcastInterface.Reserved = 0;
			return Win32Usb.RegisterDeviceNotification(hWnd, deviceBroadcastInterface, 0u);
		}

		public static bool UnregisterForUsbEvents(IntPtr hHandle)
		{
			return Win32Usb.UnregisterDeviceNotification(hHandle);
		}

		public Win32Usb()
		{
			
		}

		static Win32Usb()
		{
			
			Win32Usb.NullHandle = IntPtr.Zero;
			Win32Usb.InvalidHandleValue = new IntPtr(-1);
		}
	}
}
