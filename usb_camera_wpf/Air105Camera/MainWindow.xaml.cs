using ICSharpCode.SharpZipLib.Zip.Compression.Streams;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
using System.Drawing.Imaging;
using System.IO;
using System.IO.Compression;
using System.IO.Ports;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Interop;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using Brush = System.Drawing.Brush;
using Color = System.Drawing.Color;
using Pen = System.Drawing.Pen;
using PixelFormat = System.Drawing.Imaging.PixelFormat;
using Rectangle = System.Drawing.Rectangle;

namespace Air105Camera
{
    /// <summary>
    /// MainWindow.xaml 的交互逻辑
    /// </summary>
    [PropertyChanged.AddINotifyPropertyChangedInterface]
    public partial class MainWindow : Window
    {
        public MainWindow()
        {
            InitializeComponent();
        }

        //将Bitmap对象转换成bitmapImage对象
        private BitmapImage ConvertBitmapToBitmapImage(Bitmap bitmap)
        {
            MemoryStream stream = new MemoryStream();
            bitmap.Save(stream, ImageFormat.Bmp);
            BitmapImage image = new BitmapImage();
            image.BeginInit();
            image.StreamSource = stream;
            image.EndInit();
            return image;
        }

        //刷新显示当前图片
        private void ShowImage()
        {
            this.Dispatcher.Invoke(() =>
            {
                var temp = ConvertBitmapToBitmapImage(image);
                CameraImage.Source = temp;
            });
        }

        //缓存图片数据
        Bitmap image = new Bitmap(320, 240, PixelFormat.Format16bppRgb565);
        Graphics imageGraphic = null;
        //串口对象
        private SerialPort Uart = new SerialPort();
        //总共收到多少包
        public int totalPack { get; set; } = 0;
        //总共刷了多少张
        public int totalPic { get; set; } = 0;
        //FPS数
        public double fpsNow { get; set; } = 0;
        public bool connectEnable { get; set; } = true;
        public bool disconnectEnable { get; set; } = true;

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            this.DataContext = this;

            imageGraphic = Graphics.FromImage(image);
            //刷白
            imageGraphic.FillRectangle(new SolidBrush(Color.White), 0, 0, image.Width, image.Height);

            //图片显示一下
            ShowImage();

            // 绑定事件监听,用于监听HID设备插拔
            (PresentationSource.FromVisual(this) as HwndSource)?.AddHook(WndProc);
            RefreshPortList();

            //串口数据处理，随便写写
            new Thread(Uart2Image).Start();
        }


        private void ConnectButton_Click(object sender, RoutedEventArgs e)
        {
            if (Uart.IsOpen)
            {
                return;
            }
            if (((string)SerialComboBox.SelectedItem).Length > 0)
            {
                var portName = (string)SerialComboBox.SelectedItem;
                Task.Run(() =>
                {
                    try
                    {
                        Uart.PortName = portName;
                        Uart.Open();
                    }
                    catch (Exception ex)
                    {
                        MessageBox.Show(ex.Message);
                    }
                });
            }
        }

        private void DisconnectButton_Click(object sender, RoutedEventArgs e)
        {
            if (!Uart.IsOpen)
            {
                return;
            }
            try
            {
                Uart.Close();
            }
            catch(Exception ex)
            {
                MessageBox.Show(ex.Message);
            }
        }

        private static int UsbPluginDeley = 0;
        private IntPtr WndProc(IntPtr hwnd, int msg, IntPtr wParam, IntPtr lParam, ref bool handled)
        {
            if (msg == 0x219)// 监听USB设备插拔消息
            {
                if (UsbPluginDeley == 0)
                {
                    ++UsbPluginDeley;   // Task启动需要准备时间,这里提前对公共变量加一
                    Task.Run(() =>
                    {
                        do Task.Delay(100).Wait();
                        while (++UsbPluginDeley < 10);
                        UsbPluginDeley = 0;
                        RefreshPortList();
                    });
                }
                else UsbPluginDeley = 1;
                handled = true;
            }
            return IntPtr.Zero;
        }

        /// <summary>
        /// 刷新设备列表，并在断开恢复后重连
        /// </summary>
        /// <returns>串口列表是否成功获取</returns>
        private bool RefreshPortList()
        {
            bool result = true;
            var list = SerialPort.GetPortNames();
            Dispatcher.Invoke(delegate
            {
                SerialComboBox.ItemsSource = list;
                if (SerialComboBox.Items.Count == 0)
                {
                    result = false;
                }
                else if (SerialComboBox.Items.Contains(Uart.PortName))
                {
                    SerialComboBox.SelectedItem = Uart.PortName;
                }
                else SerialComboBox.SelectedIndex = 0;
            });
            return result;
        }

        bool closed = false;
        private void Window_Closing(object sender, System.ComponentModel.CancelEventArgs e)
        {
            closed = true;
            //直接强关软件
            Environment.Exit(0);
        }

        private void RefreshButton_Click(object sender, RoutedEventArgs e)
        {
            RefreshPortList();
        }

        /*
        16字节头+N字节数据
        4字节字符串 “VCAM” + 2字节图像宽度 + 2字节图像高度 + 4字节本次数据从哪行开始刷 + 2字节未压缩数据长度 + 2字节压缩后图像数据长度
        如果压缩前和压缩后数据长度是一致的，那就没有压缩；如果不一致，则后面的N字节要用zlib解压
        RGB565大端格式
        */
        //串口数据处理
        private void Uart2Image()
        {
            byte[] temp = new byte[10 * 1024 * 1024];//缓冲区
            int p = 0;//上次的位置
            Stopwatch sw = new Stopwatch();
            sw.Start();
            bool getNewData = false;//记一下是不是收到新数据了
            while (true)
            {
                if (closed)
                    return;
                connectEnable = !Uart.IsOpen;
                disconnectEnable = Uart.IsOpen;
                try
                {
                    if (Uart.IsOpen && Uart.BytesToRead > 0)
                    {
                        //读数据
                        var toread = Uart.BytesToRead;
                        Uart.Read(temp, p, toread);
                        p += toread;
                        getNewData = true;
                    }
                    else if (!Uart.IsOpen)
                    {
                        fpsNow = 0;
                        p = 0;
                    }
                    if(getNewData)
                    {
                        getNewData = false;//收到新数据再刷
                        for (int i = 3; i < p; i++)
                        {
                            //匹配VCAM头
                            if (temp[i - 3] == 'V' && temp[i - 2] == 'C' && temp[i - 1] == 'A' && temp[i] == 'M'
                                && p - i > 16)
                            {
                                var width = BitConverter.ToUInt16(temp, i + 1);
                                var height = BitConverter.ToUInt16(temp, i + 3);
                                var start = BitConverter.ToUInt32(temp, i + 5);
                                var rawLen = BitConverter.ToUInt16(temp, i + 9);
                                var comLen = BitConverter.ToUInt16(temp, i + 11);
                                Debug.WriteLine($"found VCAM: {BitConverter.ToString(temp, i - 3, 16)}\r\n" +
                                    $"{width},{height},{start},{rawLen},{comLen},{i},{p}");
                                //获取实际数据长度
                                var realLength = comLen < rawLen ? comLen : rawLen;
                                if (realLength <= p - i - 13)//一包数据够了
                                {
                                    totalPack++;
                                    Debug.WriteLine($"{realLength} bytes, start unpack");

                                    //存储rgb数据
                                    byte[] data = null;
                                    if (comLen < rawLen)//包压缩了
                                    {
                                        //Debug.WriteLine(BitConverter.ToString(temp, i + 13, comLen));
                                        using MemoryStream compressed = new MemoryStream(temp, i + 13, comLen);
                                        using MemoryStream decompressed = new MemoryStream();
                                        using InflaterInputStream inputStream = new InflaterInputStream(compressed);
                                        inputStream.CopyTo(decompressed);
                                        data = decompressed.ToArray();
                                    }
                                    else//没压缩
                                    {
                                        Debug.WriteLine($"rawLen,{rawLen},{i + 13 + rawLen}");
                                        data = new byte[rawLen];
                                        for (int n = i + 13; n < i + 13 + rawLen; n++)
                                            data[n - (i + 13)] = temp[n];
                                    }

                                    //直接把rgb565数据写入bitmap，这样快
                                    //先把数据改成小端
                                    for (int n = 0; n < data.Length; n += 2)
                                    {
                                        (data[n], data[n + 1]) = (data[n + 1], data[n]);
                                    }
                                    var rect = new Rectangle(0, (int)start, width, data.Length / 2 / width);
                                    BitmapData bmpData = image.LockBits(rect, ImageLockMode.ReadWrite, image.PixelFormat);
                                    IntPtr ptr = bmpData.Scan0;// Get the address of the first line.
                                    System.Runtime.InteropServices.Marshal.Copy(data, 0, ptr, data.Length);
                                    image.UnlockBits(bmpData);

                                    //刷到最后一行时再显示当前图片
                                    if (start >= height - 16)
                                    {
                                        sw.Stop();
                                        fpsNow = 1000.0 / sw.ElapsedMilliseconds;
                                        sw.Restart();
                                        totalPic++;
                                        ShowImage();
                                    }

                                    //剩下的数据全部左移到开头
                                    for (int n = i + 13 + realLength; n < p; n++)
                                        temp[n - (i + 13 + realLength)] = temp[n];
                                    p -= i + 13 + realLength;
                                    Debug.WriteLine($"done, p now {p}");
                                    if (p > 3)
                                        i = 2;
                                    else
                                        break;
                                }
                            }
                        }
                    }
                    Thread.Sleep(1);
                }
                catch (Exception e)
                {
                    Debug.WriteLine(e.Message);
                }
            }
        }
    }
}
