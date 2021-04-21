using System;
using System.Runtime.InteropServices;

namespace UsbLibrary
{
	public class GException0 : ApplicationException
	{
		public GException0(string strMessage)
		{
			
			//base._002Ector(strMessage);
		}

		public static GException0 GenerateWithWinError(string strMessage)
		{
			return new GException0(string.Format("Msg:{0} WinEr:{1:X8}", strMessage, Marshal.GetLastWin32Error()));
		}

		public static GException0 GenerateError(string strMessage)
		{
			return new GException0(string.Format("Msg:{0}", strMessage));
		}
	}
}
