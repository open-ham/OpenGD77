/*
 * Modified version by Roger VK3KYY - converts bin to SGL file by prepending the header
 * and encrypting the binary
 * 
 * Original version...
 * GD-77 firmware decrypter/encrypter by DG4KLU.
 *
 * Copyright (C)2019 Kai Ludwig, DG4KLU
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
using System.IO;
using System.Collections.Generic;

namespace bin2sgl
{
    class Program
	{
		public enum OutputType
		{
			OutputType_GD77,
			OutputType_GD77S,
			OutputType_DM1801,
			OutputType_RD5R
		}

		private static OutputType outputType = OutputType.OutputType_GD77;

		static void encrypt(string[] args)
        {
			int shift = 0;
			int flength = 0;
            FileStream stream_fw_in;
            string outputFilename = Path.GetFileNameWithoutExtension(args[0]) + ".sgl";

			switch (outputType)
			{
				case OutputType.OutputType_GD77:
					shift = 0x0807;
					flength = 0x77001; // The header, from firmware version 3.1.8 expects the file to be 0x77001 long
					Console.WriteLine("GD77");
					break;

				case OutputType.OutputType_GD77S:
					shift = 0x2a8e;
					flength = 0x77001; // The header, from firmware version 1.2.0 expects the file to be 0x50001 long, but it has been hacked to 0x77001
					Console.WriteLine("GD77S");
					break;

				case OutputType.OutputType_DM1801:
					shift = 0x2C7C;
					flength = 0x78001; // The header, from firmware version 2.1.9 expects the file to be 0x78001 long
					Console.WriteLine("DM-1801");
					break;

				case OutputType.OutputType_RD5R:
					shift = 0x306E;
					flength = 0x78001; // The header, from firmware version 2.1.7 expects the file to be 0x78001 long
					Console.WriteLine("RD-5R");
					break;
			}

			try
            {
                try
                {
                    stream_fw_in = new FileStream(args[0], FileMode.Open, FileAccess.Read, FileShare.ReadWrite);
                }
                catch(Exception)
                {
                    Console.Write("Error: Unable to open file " + args[0]);
                    return;
                }
                FileStream stream_fw_out = new FileStream(outputFilename, FileMode.Create, FileAccess.Write, FileShare.ReadWrite);

				// Copy header
				switch (outputType)
				{
					case OutputType.OutputType_GD77:
						for (int i = 0; i < DataArrays.Header318_0x0807.Length; i++)
						{
							stream_fw_out.WriteByte(DataArrays.Header318_0x0807[i]);
						}
						break;

					case OutputType.OutputType_GD77S:
						for (int i = 0; i < DataArrays.Header120_0x2a8e.Length; i++)
						{
							stream_fw_out.WriteByte(DataArrays.Header120_0x2a8e[i]);
						}
						break;

					case OutputType.OutputType_DM1801:
						for (int i = 0; i < DataArrays.Header219_0x2c7c.Length; i++)
						{
							stream_fw_out.WriteByte(DataArrays.Header219_0x2c7c[i]);
						}
						break;

					case OutputType.OutputType_RD5R:
						for (int i = 0; i < DataArrays.Header217_0x306e.Length; i++)
						{
							stream_fw_out.WriteByte(DataArrays.Header217_0x306e[i]);
						}
						break;
				}

				int length = 0;
                while (length < flength)
                {
                    int data = stream_fw_in.ReadByte();// This may attempt to read past of the end of the file if its shorter than 0x77001

                    // if readByte was beyond the end of the file, we pad with 0xff
                    if (data < 0)
                    {
                        data = 0xFF;
                    }
                    length++;

                    data = (byte)data ^ DataArrays.EncryptionTable[shift++];
                    data = ~(((data >> 3) & 0b00011111) | ((data << 5) & 0b11100000));

                    stream_fw_out.WriteByte((byte)data);

                    if (shift >= 0x7fff)
                    {
                        shift = 0;
                    }
                }

                stream_fw_in.Close();
                stream_fw_out.Close();
                Console.WriteLine("Created " + outputFilename);
            }
            catch(Exception ex)
            {
                Console.Write("Error :-(\n" + ex.ToString());
            }
		}

		private static string[] RemoveArgAt(string[] args, int index)
		{
			var foos = new List<string>(args);
			foos.RemoveAt(index);
			return foos.ToArray();
		}

		static void Main(string[] args)
		{
			int idxDM1801 = Array.IndexOf(args, "DM-1801");
			int idxGD77S = Array.IndexOf(args, "GD-77S");
			int idxRD5R = Array.IndexOf(args, "RD-5R");

			outputType = OutputType.OutputType_GD77; // Default platform target

			if (idxGD77S >= 0)
			{
				outputType = OutputType.OutputType_GD77S;
				args = RemoveArgAt(args, idxGD77S);
			}
			else if (idxDM1801 >= 0)
			{
				outputType = OutputType.OutputType_DM1801;
				args = RemoveArgAt(args, idxDM1801);
			}
			else if (idxRD5R >= 0)
			{
				outputType = OutputType.OutputType_RD5R;
				args = RemoveArgAt(args, idxRD5R);
			}

			if (args.Length > 0)
            {
                encrypt(args);
            }
            else
            {
                Console.WriteLine("Usage: bin2sgl [DM-1801 | GD-77S | RD-5R] filename");
            }
        }
    }
}
