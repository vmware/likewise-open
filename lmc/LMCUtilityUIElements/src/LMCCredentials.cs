using System;
using System.Collections.Generic;
using System.Text;
using System.IO;
using System.Windows.Forms;
using Likewise.LMC.ServerControl;
using Likewise.LMC.Utilities;

namespace Likewise.LMC.UtilityUIElements
{
    public class LMCCredentials : Credentials
    {
        #region Class Data

        private static IPlugIn _plugin = null;
        private static Hostinfo _hn = null;
        private static string sCredentialsFilePath = Path.Combine(Application.UserAppDataPath, @"LMCCredentials.ini");

        #endregion

        #region Constructors

        /// <summary>
        /// Use this constructor to set the default credentilas
        /// </summary>
        public LMCCredentials()
            : base()
        {

        }

        /// <summary>
        /// Use this constructor to set the credentails given by the user. If username or domain is null then it will treat
        /// it as using of default credentails
        /// </summary>
        /// <param name="username"></param>
        /// <param name="password"></param>
        /// <param name="domain"></param>
        /// <param name="hostname"></param>
        public LMCCredentials(string username, string password, string domain, string hostname)
            : base(username, password, domain, hostname)
        {

        }

        /// <summary>
        /// Use this contructor when we want to read the credentails from .ini file and
        /// use them as default credentilas for the dialog
        /// </summary>
        /// <param name="plugin"></param>
        /// <param name="hn"></param>
        public LMCCredentials(IPlugIn plugin, Hostinfo hn)
            : base(hn.creds.UserName, hn.creds.Password, hn.creds.Domain, hn.hostName)
        {
            _plugin = plugin;
            _hn = hn;

            if (IsConnectSetBefore(plugin)) {
                GetConnectedHostInfoFromIni(_plugin, _hn);

                this.Domain = hn.domainName;
                this.Hostname = hn.hostName;
                this.Username = hn.creds.UserName;
                this.Password = hn.creds.Password;
            }
        }

        #endregion

        #region Public functions

        private static bool IsConnectSetBefore(IPlugIn requestor)
        {
            bool IsSetBefore = false;
            bool IsFileExists = !string.IsNullOrEmpty(sCredentialsFilePath) && Path.IsPathRooted(sCredentialsFilePath) && File.Exists(sCredentialsFilePath);

            if (!IsFileExists)
                return IsSetBefore;

            StreamReader reader = new StreamReader(sCredentialsFilePath);

            if (!reader.EndOfStream)
            {
                string fileContent = reader.ReadToEnd();
                if (fileContent != null && fileContent.Trim().Contains(requestor.GetName())) {
                    IsSetBefore = true;
                }
            }
            reader.Close();

            return IsSetBefore;
        }


        /// <summary>
        /// Reads the target machine info for the current plugin from credentilas ini and assign to Hostinfo
        /// </summary>
        /// <param name="requestor"></param>
        /// <param name="_hn"></param>
        public static void GetConnectedHostInfoFromIni(IPlugIn requestor, Hostinfo _hn)
        {
            string sPlugInName = requestor.GetName();

            bool IsFileExists = !string.IsNullOrEmpty(sCredentialsFilePath) && Path.IsPathRooted(sCredentialsFilePath) && File.Exists(sCredentialsFilePath);

            if (!IsFileExists)
                return;

            StreamReader reader = new StreamReader(sCredentialsFilePath);

            while (!reader.EndOfStream)
            {
                string currentLine = reader.ReadLine();
                if (currentLine != null && currentLine.Trim().Equals(sPlugInName))
                {
                    currentLine = reader.ReadLine();

                    int index = 0;
                    if (currentLine != null && currentLine.Trim().IndexOf("hostname=") >= 0 &&
                        String.IsNullOrEmpty(_hn.hostName))
                    {
                        currentLine = currentLine.Trim();
                        index = (currentLine.IndexOf('=') + 1);
                        _hn.hostName = currentLine.Substring(index, (currentLine.Length - index));
                        currentLine = reader.ReadLine();
                    }

                    if (currentLine != null && currentLine.Trim().IndexOf("username=") >= 0 &&
                        String.IsNullOrEmpty(_hn.creds.UserName))
                    {
                        currentLine = currentLine.Trim();
                        index = (currentLine.IndexOf('=') + 1);
                        _hn.creds.UserName = currentLine.Substring(index, (currentLine.Length - index));
                        currentLine = reader.ReadLine();
                    }

                    if (currentLine != null && currentLine.Trim().IndexOf("domainFQDN=") >= 0 &&
                        String.IsNullOrEmpty(_hn.domainName))
                    {
                        currentLine = currentLine.Trim();
                        index = (currentLine.IndexOf('=') + 1);
                        _hn.domainName = currentLine.Substring(index, (currentLine.Length - index));
                        currentLine = reader.ReadLine();
                    }

                    if (currentLine != null && currentLine.IndexOf("domainShort=") >= 0 &&
                        String.IsNullOrEmpty(_hn.creds.Domain))
                    {
                        currentLine = currentLine.Trim();
                        index = (currentLine.IndexOf('=') + 1);
                        _hn.creds.Domain = currentLine.Substring(index, (currentLine.Length - index));
                    }
                    break;
                }
            }
            reader.Close();
        }

        /// <summary>
        /// Method to create the ini file, if doesn't exits.
        /// </summary>
        /// <param name="Path"></param>
        private void CreateCredentialsIni(string Path)
        {
            FileStream fileStream = null;
            try
            {
                Logger.Log(String.Format(
                    "LMCCredentials.CreateCredentialsIni: creating {0}", Path),
                    Logger.manageLogLevel);
                fileStream = new FileStream(Path, FileMode.OpenOrCreate, FileAccess.ReadWrite);
            }
            catch (Exception)
            {
                Logger.Log(String.Format(
                    "LMCCredentials.CreateCredentialsIni: failed to create {0}", Path),
                    Logger.manageLogLevel);
            }
            finally
            {
                if (fileStream != null)
                    fileStream.Close();
            }
        }

        /// <summary>
        /// Method to store the plugin's host info to ini file.
        /// </summary>
        /// <param name="_hn"></param>
        private void SaveTargetMachineInfoToIni(Hostinfo _hn, IPlugIn plugin)
        {
            Logger.Log(String.Format(
                "LMCCredentials.SaveTargetMachineInfoToIni: saving the target machine info to ini: _hn =\n {0}",
                _hn == null ? "<null>" : _hn.ToString()),
                Logger.manageLogLevel);

            string sPath = Path.Combine(Application.UserAppDataPath, @"Temp.ini");

            CreateCredentialsIni(sCredentialsFilePath);

            StreamReader reader = new StreamReader(sCredentialsFilePath);
            FileStream fStream = new FileStream(sPath, FileMode.OpenOrCreate, FileAccess.ReadWrite, FileShare.ReadWrite);
            StreamWriter writer = new StreamWriter(fStream);

            string sPlugInName = plugin.GetName();

            Logger.Log(String.Format(
                "LMCCredentials.SaveTargetMachineInfoToIni: plugin name={0}",
                sPlugInName == null ? "<null>" : sPlugInName),
                Logger.manageLogLevel);

            if (!String.IsNullOrEmpty(sPlugInName))
            {
                string currentLine = reader.ReadLine();

                while (!(currentLine != null && currentLine.Trim().Equals(sPlugInName)) &&
                       !reader.EndOfStream)
                {
                    writer.WriteLine(currentLine);
                    currentLine = reader.ReadLine();
                }

                writer.WriteLine(sPlugInName);
                if (!String.IsNullOrEmpty(_hn.hostName))
                {
                    writer.WriteLine("hostname=" + _hn.hostName.Trim());
                }
                if (_hn.creds != null && !String.IsNullOrEmpty(_hn.creds.UserName))
                {
                    writer.WriteLine("username=" + _hn.creds.UserName.Trim());
                }
                if (!String.IsNullOrEmpty(_hn.domainName))
                {
                    writer.WriteLine("domainFQDN=" + _hn.domainName.Trim());
                }
                if (_hn.creds != null && !String.IsNullOrEmpty(_hn.creds.Domain))
                {
                    writer.WriteLine("domainShort=" + _hn.creds.Domain.Trim());
                }
                writer.WriteLine("");

                //make sure the remainder of the old file is present in the new one.
                currentLine = reader.ReadLine();
                while (!(currentLine != null && currentLine.Trim().Length > 0 && currentLine.Trim()[0] == '[') &&
                        !reader.EndOfStream)
                {
                    currentLine = reader.ReadLine();
                }
                while (!reader.EndOfStream)
                {
                    writer.WriteLine(currentLine);
                    currentLine = reader.ReadLine();
                }
                if (reader.EndOfStream)
                {
                    writer.WriteLine("");
                }
            }
            writer.Flush();

            writer.Close();
            reader.Close();
            fStream.Close();
            fStream.Dispose();

            if (File.Exists(sCredentialsFilePath) && File.Exists(sPath))
            {
                File.Delete(sCredentialsFilePath);
                File.Move(sPath, sCredentialsFilePath);
            }
        }

        #endregion

        #region Access Specifiers

        public static Hostinfo HostInfo
        {
            set {
                _hn = value;
            }
            get {
                if (_hn == null)
                    _hn = new Hostinfo();

                return _hn;
            }
        }

        #endregion
    }
}
