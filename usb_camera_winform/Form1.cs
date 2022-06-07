using System.Collections.Generic;
using System.IO.Ports;
using System.Text.RegularExpressions;

namespace Air105摄像头预览
{
    public partial class Form1 : Form
    {
        Thread thread;
        SerialPort serialPort = new SerialPort();
        string comName = "COM91";
        bool power_ready = false;
        int com_br = 1500000;
        public Form1()
        {
            InitializeComponent();
            thread = new Thread(Run_mac_flasher);
            thread.Name = "com_reader";
            thread.IsBackground = true;
        }

        private void Form1_Load(object sender, EventArgs e)
        {
            reload_com_names();
            thread.Start();
        }
        void reload_com_names()
        {
            comboBox_coms.Items.Clear();
            foreach (string com in System.IO.Ports.SerialPort.GetPortNames())
            {
                comboBox_coms.Items.Add(com);
            }
            if (comboBox_coms.Items.Count > 0)
            {
                comboBox_coms.SelectedIndex = 0;
                //richTextBox_logs.Text = "找到" + comboBox_coms.Items.Count + "个串口";
            }
            else
            {
                //richTextBox_logs.Text = "没有任何串口";
            }
        }

        void show_log(string text)
        {
            this.label_data_log.BeginInvoke(new Action(() => {
                this.label_data_log.Text = text;
            }));
        }

        void Run_mac_flasher()
        {
            Thread.Sleep(1000);
            var buff = new byte[64 * 1024];
            while (true)
            {
                if (comName == "" || power_ready == false)
                {
                    Thread.Sleep(100);
                    continue;
                }
                if (!serialPort.IsOpen)
                {
                    try
                    {
                        serialPort.PortName = comName;
                        serialPort.BaudRate = this.com_br;
                        serialPort.DataBits = 8;
                        serialPort.StopBits = StopBits.One;
                        serialPort.Parity = Parity.None;
                        serialPort.Open();
                    }
                    catch (Exception ex)
                    {
                        show_log("打开串口异常 " + comName + " " + ex);
                        Thread.Sleep(1000);
                        continue;
                    }
                }
                var rlen = serialPort.Read(buff, 0, buff.Length);
                var dataHeader = "Air105 USB JPG ";
                var text = "";
                for (int i = 0; i < buff.Length; i++)
                {
                    //buff[i] = 0;
                }
                if (rlen > 0)
                {
                    if (buff[0] == 'A' && buff[1] == 'i' && buff[2] == 'r')
                    {
                        for (int i = 0; i < buff.Length; i++)
                        {
                            if (buff[i] == '\r' && buff[i+1] == '\n')
                            {
                                var head = System.Text.Encoding.UTF8.GetString(buff, 0, i);
                                if (!head.StartsWith(dataHeader))
                                {
                                    break;
                                }
                                // 解析出长度
                                var tmp = head.Substring(dataHeader.Length);
                                var dataRequire = 0;
                                try
                                {
                                    dataRequire = Int32.Parse(tmp);
                                }
                                catch (FormatException)
                                {
                                    text = "错误的字符串" + tmp + " >> " + head;
                                    this.label_data_log.BeginInvoke(new Action(() =>
                                    {
                                        this.label_data_log.Text = text;
                                    }));
                                    break;
                                }
                                //var dataRecv = rlen;
                                while (rlen < dataRequire + i + 2)
                                {
                                    var len2 = serialPort.Read(buff, rlen, 8192);
                                    if (len2 > 0)
                                    {
                                        rlen += len2;
                                    }
                                    else if (len2 < 0)
                                    {
                                        break;
                                    }
                                    //Thread.Sleep(1);
                                }
                                text = "期待长度 " + (dataRequire  + i + 2) + " 总共读取 " + rlen;
                                var tmpbuff = new byte[dataRequire];
                                for (int z = 0; z < dataRequire; z++)
                                {
                                    tmpbuff[z] = buff[i + 2 + z];
                                }
                                File.WriteAllBytes("temp.jpg", tmpbuff);
                                System.Drawing.Image image = null;
                                try
                                {
                                    image = System.Drawing.Image.FromStream(new MemoryStream(buff, i + 2, dataRequire));
                                    text = "图片解码成功 长度" + dataRequire + "字节";
                                }
                                catch
                                {
                                    text = "图片不合法";
                                }
                                this.label_data_log.BeginInvoke(new Action(() =>
                                {
                                    this.label_data_log.Text = text;
                                    if (image != null)
                                        this.pictureBox_main.Image = image;
                                }));
                                break;
                            }
                        }
                    }
                }
                Thread.Sleep(100);
            }
        }

        private void label_comName_Click(object sender, EventArgs e)
        {

        }

        private void button2_Click(object sender, EventArgs e)
        {
            reload_com_names();
        }

        private void button1_Click(object sender, EventArgs e)
        {
            if (power_ready)
            {
                power_ready = false;
                this.button_power.Text = "开始读取";
                this.button_power.BackColor = Color.Green;
                return;
            }
            if (this.comboBox_coms.SelectedIndex < 0 || this.comboBox_coms.Items[comboBox_coms.SelectedIndex] == null)
            {
                comName = "";
                MessageBox.Show(this, "请先刷新并选择串口");
                return;
            }
            try
            {
                this.com_br = Int32.Parse(this.textBox_br.Text);
            }
            catch (Exception ex)
            {
                MessageBox.Show(this, "波特率不合法,请检查");
                return;
            }
            this.power_ready = true;
            this.button_power.Text = "停止读取";
            this.comName = this.comboBox_coms.Items[comboBox_coms.SelectedIndex].ToString();
            this.button_power.BackColor = Color.Red;
        }
    }
}