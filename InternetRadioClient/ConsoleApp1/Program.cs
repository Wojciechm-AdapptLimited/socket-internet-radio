using System.Net;
using System.Net.Sockets;

Console.WriteLine("Hello World!");
var ipHostInfo = await Dns.GetHostEntryAsync("127.0.0.1");
var ipAddress = ipHostInfo.AddressList[0];
var ipEndPoint = new IPEndPoint(ipAddress, 1100);
var buffer = new ArraySegment<byte>(new byte[1024]);
var headerBuffer = new ArraySegment<byte>(new byte[1]);

using Socket client = new(ipEndPoint.AddressFamily, SocketType.Stream, ProtocolType.Tcp);
await client.ConnectAsync(ipEndPoint);
Console.WriteLine("connect to server ");

while (true)
{
    var headerResult = await client.ReceiveAsync(headerBuffer, SocketFlags.None);

    if (headerResult == 0)
        break;

    var dataType = headerBuffer[0];
    
    if (await client.ReceiveAsync(buffer, SocketFlags.None) == 0)
    {
        Console.WriteLine("Disconnection");
        break;
    }
}
