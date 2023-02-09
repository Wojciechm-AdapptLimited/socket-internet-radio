using System.Collections.Concurrent;
using System.Net;
using System.Net.Sockets;
using System.Net.WebSockets;

namespace InternetRadioClient.Services;

public class ConnectionService
{
    private const int BufferSize = 1024;
    private readonly ConcurrentQueue<byte[]?> _audioBuffer = new();
    private readonly CancellationTokenSource _cancellationTokenSource = new();
    
    public async Task ConnectAsync(ClientWebSocket socket, Uri serverUri, CancellationToken cancellationToken)
    {
        _cancellationTokenSource.CancelAfter(5000);
        Console.WriteLine("hello1");
        await socket.ConnectAsync(serverUri, cancellationToken);
        Console.WriteLine("hello2");
        _ = ReceiveAudioAsync(socket, cancellationToken);
    }
    
    public async Task DisconnectAsync(ClientWebSocket socket, CancellationToken cancellationToken)
    {
        Console.WriteLine("Hello disconnect");
        if (socket.State == WebSocketState.Open)
        {
            Console.WriteLine("Hello disconnect 2");
            await socket.CloseAsync(WebSocketCloseStatus.NormalClosure, "Disconnecting", cancellationToken);
        }
    }
    
    public bool TryDequeueAudio(out byte[]? audioData)
    {
        return _audioBuffer.TryDequeue(out audioData);
    }
    
    private async Task ReceiveAudioAsync(ClientWebSocket socket, CancellationToken cancellationToken)
    {
        Console.WriteLine("hello");
        var buffer = new ArraySegment<byte>(new byte[BufferSize]);
        
        while (true)
        {
            var result = await socket.ReceiveAsync(buffer, cancellationToken);
            if (result.MessageType == WebSocketMessageType.Close)
            {
                await DisconnectAsync(socket, cancellationToken);
                break;
            }

            var audioData = new byte[result.Count];
            if (buffer.Array != null) Array.Copy(buffer.Array, audioData, result.Count);
            _audioBuffer.Enqueue(audioData);
        }
    }
}