using System.Collections.Concurrent;
using System.Net;
using System.Net.Sockets;

namespace InternetRadioClient.Services;

public class ConnectionService
{
    private const int BufferSize = 1024;
    private const int HeaderLength = 1;
    private readonly ConcurrentQueue<byte[]?> _audioBuffer = new();
    private Socket? _socket;

    public async Task ConnectAsync(string serverAddress, int port, CancellationToken cancellationToken)
    {
        var ipAddress = IPAddress.Parse(serverAddress);
        var ipEndPoint = new IPEndPoint(ipAddress, port);
        _socket = new Socket(ipEndPoint.AddressFamily, SocketType.Stream, ProtocolType.Tcp);
        try
        {
            await _socket.ConnectAsync(ipEndPoint, cancellationToken);
            _ = ReceiveAudioAsync(cancellationToken);
        }
        catch (Exception e)
        {
            Console.WriteLine("Failed to connect to the server: " + e.Message);
            _socket.Shutdown(SocketShutdown.Both);
            _socket = null;
            throw;
        }
    }
    
    public async Task DisconnectAsync(CancellationToken cancellationToken)
    {
        if (_socket is { Connected: true })
        {
            try
            {
                await _socket.DisconnectAsync(false, cancellationToken);
            }
            catch (Exception e)
            {
                Console.WriteLine("Failed to disconnect from the server: " + e.Message);
            }
            finally
            {
                _socket.Shutdown(SocketShutdown.Both);
            }
        }
    }

    public bool TryDequeueAudio(out byte[]? audioData)
    {
        return _audioBuffer.TryDequeue(out audioData);
    }
    
    private async Task ReceiveAudioAsync(CancellationToken cancellationToken)
    {
        if (_socket == null)
                return;

        var buffer = new ArraySegment<byte>(new byte[BufferSize]);
        var headerBuffer = new ArraySegment<byte>(new byte[HeaderLength]);
        
        while (!cancellationToken.IsCancellationRequested)
        {
            try
            {
                var headerResult = await _socket.ReceiveAsync(headerBuffer, cancellationToken);

                if (headerResult == 0)
                    break;

                var dataType = headerBuffer[0];
                
                var result = await _socket.ReceiveAsync(buffer, SocketFlags.None, cancellationToken);

                if (result == 0)
                    break;

                if (dataType == 1)
                {
                    var audioData = new byte[result];
                    Array.Copy(buffer.Array!, audioData, result);
                    _audioBuffer.Enqueue(audioData);
                }
                else
                {
                    
                }
            }
            catch (Exception e)
            {
                Console.WriteLine("Failed to receive audio data: " + e.Message);
                break;
            }
        }
        
        await DisconnectAsync(CancellationToken.None);
    }
}