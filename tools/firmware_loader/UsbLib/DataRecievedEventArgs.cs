using System;

namespace UsbLibrary
{
	public class DataRecievedEventArgs : EventArgs
	{
		public readonly byte[] data;

		public DataRecievedEventArgs(byte[] data)
		{
			
			//base._002Ector();
			this.data = data;
		}
	}
}
