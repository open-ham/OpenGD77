using System;
using System.Linq;
using System.Collections.Generic;
using System.Threading.Tasks;
using System.Windows.Forms;


namespace GD77_FirmwareLoader
{
	static class Program
	{
		private static string[] RemoveArgAt(string[] args, int index)
		{
			var foos = new List<string>(args);
			foos.RemoveAt(index);
			return foos.ToArray();
		}

		/// <summary>
		/// The main entry point for the application.
		/// </summary>
		[STAThread]
		static void Main(string[] args)
		{
			int exitCode = 0;
			/* Testing only
			   args = new string[2];
			   args[0] = "test.bin";
			   args[1] = "GUI";
			*/
			if (args.Length == 0)
			{
				//				FirmwareLoader.outputType = FirmwareLoader.OutputType.OutputType_UNKOWN; //FirmwareLoader.probeModel();
				//FirmwareLoader.outputType = FirmwareLoader.OutputType.OutputType_GD77;// Probe is not currently working, so default to the GD-77

				/*				
				if ((FirmwareLoader.outputType < FirmwareLoader.OutputType.OutputType_GD77) || (FirmwareLoader.outputType > FirmwareLoader.OutputType.OutputType_RD5R))
				{
					Console.WriteLine("Unable to detect HT model, using GD-77 as fallback.");
					FirmwareLoader.outputType = FirmwareLoader.OutputType.OutputType_GD77;
				}
				else
				{
					Console.WriteLine(String.Format("Detected mode: {0}", FirmwareLoader.getModelName()));
				}
				*/

				Application.EnableVisualStyles();
				Application.SetCompatibleTextRenderingDefault(false);
				Application.Run(new MainForm());
			}
			else
			{
				if (args.Contains("--help") || args.Contains("-h") || args.Contains("/h"))
				{
					//FirmwareLoader.OutputType[] models = new FirmwareLoader.OutputType[] { FirmwareLoader.OutputType.OutputType_GD77, FirmwareLoader.OutputType.OutputType_GD77S, FirmwareLoader.OutputType.OutputType_DM1801 };
					String[] modelsString = {
						FirmwareLoader.getModelString(FirmwareLoader.OutputType.OutputType_GD77),
						FirmwareLoader.getModelString(FirmwareLoader.OutputType.OutputType_GD77S),
						FirmwareLoader.getModelString(FirmwareLoader.OutputType.OutputType_DM1801),
						FirmwareLoader.getModelString(FirmwareLoader.OutputType.OutputType_RD5R)
						};
					String allModels = String.Join(" | ", modelsString);

					Console.WriteLine(String.Format("\nUsage: GD77_FirmwareLoader [GUI] [{0}] [filename]\n\n", allModels));
					Environment.Exit(exitCode);
				}

				int idxGD77 = Array.IndexOf(args, "GD-77");
				int idxDM1801 = Array.IndexOf(args, "DM-1801");
				int idxGD77S = Array.IndexOf(args, "GD-77S");
				int idxRD5R = Array.IndexOf(args, "RD-5R");

				if (idxGD77 >= 0)
				{
					FirmwareLoader.outputType = FirmwareLoader.OutputType.OutputType_GD77;
					args = RemoveArgAt(args, idxGD77);
				}
				else if (idxGD77S >= 0)
				{
					FirmwareLoader.outputType = FirmwareLoader.OutputType.OutputType_GD77S;
					args = RemoveArgAt(args, idxGD77S);
				}
				else if (idxDM1801 >= 0)
				{
					FirmwareLoader.outputType = FirmwareLoader.OutputType.OutputType_DM1801;
					args = RemoveArgAt(args, idxDM1801);
				}
				else if (idxRD5R >= 0)
				{
					FirmwareLoader.outputType = FirmwareLoader.OutputType.OutputType_RD5R;
					args = RemoveArgAt(args, idxRD5R);
				}
				else if (FirmwareLoader.outputType == FirmwareLoader.OutputType.OutputType_UNKNOWN)
				{
					String[] modelsString = {
						FirmwareLoader.getModelString(FirmwareLoader.OutputType.OutputType_GD77),
						FirmwareLoader.getModelString(FirmwareLoader.OutputType.OutputType_GD77S),
						FirmwareLoader.getModelString(FirmwareLoader.OutputType.OutputType_DM1801),
						FirmwareLoader.getModelString(FirmwareLoader.OutputType.OutputType_RD5R)
						};
					String allModels = String.Join(", ", modelsString); 
					Console.WriteLine(String.Format("Please specify one radio model from: {0}.", allModels));
					Environment.Exit(-1);
				}

				if (args.Length == 0)
				{
					Application.EnableVisualStyles();
					Application.SetCompatibleTextRenderingDefault(false);

					Application.Run(new MainForm());
				}

				int idx = Array.IndexOf(args, "GUI");
				if (idx >= 0)
				{
					args = RemoveArgAt(args, idx);

					if (args.Length <= 0)
					{
						Console.WriteLine("ERROR: No filename specified.");
						Environment.Exit(-1);
					}

					FrmProgress frmProgress = new FrmProgress();
					frmProgress.SetLabel("");
					frmProgress.SetProgressPercentage(0);
					frmProgress.Show();

					exitCode = FirmwareLoader.UploadFirmware(args[0], frmProgress);
					frmProgress.Close();
				}
				else
				{
					if (args.Length <= 0)
					{
						Console.WriteLine("ERROR: No filename specified.");
						Environment.Exit(-1);
					}

					exitCode = FirmwareLoader.UploadFirmware(args[0]);
				}
			}
			//	Console.WriteLine("Usage GD77_FirmwareLoader filename");

			Environment.Exit(exitCode);
		}
	}
}
