using System;
using System.Collections.Generic;// For List
using System.Windows.Forms;
using Newtonsoft.Json;
using System.Drawing;// for the icon

namespace GD77_FirmwareLoader
{
	public partial class FirmwareLoaderReleasesList : Form
	{
		public string SelectedURL = "";
		public string SelectedVersion = "";

		public FirmwareLoaderReleasesList(string downloadedJsonString)
		{
			InitializeComponent();
			this.Icon = Icon.ExtractAssociatedIcon(Application.ExecutablePath);// Roger Clark. Added correct icon on main form!

			List<GithubRelease> releases = JsonConvert.DeserializeObject<List<GithubRelease>>(downloadedJsonString);

			string patternFormat = "OpenGD77.sgl";
			switch (FirmwareLoader.outputType)
			{
				case FirmwareLoader.OutputType.OutputType_GD77:
					patternFormat = @"OpenGD77.sgl";
					break;
				case FirmwareLoader.OutputType.OutputType_GD77S:
					patternFormat = @"OpenGD77S.sgl";
					break;
				case FirmwareLoader.OutputType.OutputType_DM1801:
					patternFormat = @"OpenDM1801.sgl";
					break;
				case FirmwareLoader.OutputType.OutputType_RD5R:
					patternFormat = @"OpenRD5R.sgl";
					break;
			}

			foreach (GithubRelease release in releases)
			{
				foreach (GithubReleaseAssets asset in release.assets)
				{
					if (asset.browser_download_url.IndexOf(patternFormat) != -1)
					{
						int newRow = releasesGridView.Rows.Add(release.published_at.Replace("T", " "),
																release.tag_name,
																release.prerelease == false ? "Stable" : "Beta",
																release.name, asset.download_count);
						releasesGridView.Rows[newRow].Tag = new ReleaseAndAsset(release, asset);
					}
				}
			}

			releasesGridView.ReadOnly = true;

			if (releasesGridView.Rows.Count > 0)
			{
				releasesGridView.Rows[0].Selected = true;

			}
		}
		private void btnDown_Click(object sender, EventArgs e)
		{
			DataGridViewSelectedCellCollection cells = releasesGridView.SelectedCells;
			if (cells.Count > 0)
			{
				DataGridViewRow selectedRow = releasesGridView.Rows[cells[0].RowIndex];
				ReleaseAndAsset releaseAndAsset = selectedRow.Tag as ReleaseAndAsset;
				SelectedURL = releaseAndAsset.Asset.browser_download_url;
				SelectedVersion = releaseAndAsset.Release.tag_name;
				this.DialogResult = DialogResult.OK;
				this.Close();
			}
		}

		private void btnCancel_Click(object sender, EventArgs e)
		{
			this.DialogResult = DialogResult.Cancel;
			this.Close();
		}
	}
	public class GithubReleaseAuthor
	{
		public string login { get; set; }
		public int id { get; set; }
		public string node_id { get; set; }
		public string avatar_url { get; set; }
		public string gravatar_id { get; set; }
		public string url { get; set; }
		public string html_url { get; set; }
		public string followers_url { get; set; }
		public string following_url { get; set; }
		public string gists_url { get; set; }
		public string starred_url { get; set; }
		public string subscriptions_url { get; set; }
		public string organizations_url { get; set; }
		public string repos_url { get; set; }
		public string events_url { get; set; }
		public string received_events_url { get; set; }
		public string type { get; set; }

		public bool site_admin { get; set; }
	}

	public class GithubReleaseAssets
	{
		public string url { get; set; }
		public int id { get; set; }
		public string node_id { get; set; }
		public string name { get; set; }

		public object label { get; set; }
		public object uploader { get; set; }
		public string content_type { get; set; }
		public string state { get; set; }
		public int size { get; set; }
		public int download_count { get; set; }
		public string created_at { get; set; }
		public string browser_download_url { get; set; }
	}

	public class GithubRelease
	{
		public string url { get; set; }
		public string assets_url { get; set; }
		public string upload_url { get; set; }
		public string html_url { get; set; }
		public int id { get; set; }
		public string node_id { get; set; }
		public string tag_name { get; set; }
		public string target_commitish { get; set; }
		public string name { get; set; }
		public bool draft { get; set; }
		public GithubReleaseAuthor author { get; set; }
		public bool prerelease { get; set; }
		public string created_at { get; set; }
		public string published_at { get; set; }
		public List<GithubReleaseAssets> assets { get; set; }
		public string tarball_url { get; set; }
		public string zipball_url { get; set; }
		public string body { get; set; }
	}

	public class ReleaseAndAsset
	{
		public GithubRelease Release { get; set; }
		public GithubReleaseAssets Asset { get; set; }


		public ReleaseAndAsset(GithubRelease release, GithubReleaseAssets asset)
		{
			this.Release = release;
			this.Asset = asset;
		}

	}

}
