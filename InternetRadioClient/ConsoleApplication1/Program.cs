namespace ConsoleApplication1
{
    internal class Program
    {
        public static void Main(string[] args)
        {
            Console.WriteLine("Hello World!"); 
            String server = "127.0.0.1";
            Â 
            byte[] byteBuffer = Encoding.ASCII.GetBytes("Hello there");
            Â 
            int serverPort = 1100;
            Â 
            TcpClient tcpClient = null;
            NetworkStream ns = null;
            Â 
            try
            {
                tcpClient = new TcpClient(server, serverPort);
                Â 
                Console.WriteLine("connect to server ");
                Â 
                    ns = tcpClient.GetStream();
                Â 
                ns.Write(byteBuffer, 0, byteBuffer.Length);
                Â 
                Console.WriteLine("Sent to server ");
                Â 
                int totalBytesRcvd = 0;
                int bytesRcvd = 0;
                Â 
                while (totalBytesRcvd < byteBuffer.Length)
                {
                    if ((bytesRcvd = ns.Read(byteBuffer, totalBytesRcvd, byteBuffer.Length - totalBytesRcvd)) == 0)
                    {
                        Console.WriteLine("Disconnection");
                        break;
                    }
                    totalBytesRcvd += bytesRcvd;
                    Â 
                }
                Console.WriteLine("Recived " + Encoding.ASCII.GetString(byteBuffer, 0, totalBytesRcvd));
            }
            catch (Exception e)
            {
                Console.WriteLine(e.Message);
            }
            finally
            {
                ns.Close();
                tcpClient.Close();
            }
            Â 
                Â 
        }
    }
}