using System;
using System.Diagnostics;
using System.Linq;
using System.Threading;
using Thrift.Protocol;

namespace Thrift.Client.Run
{
    internal static class Run
    {
        private static void Main()
        {
            var client = new ThriftClient();
            new Thread(client.Run) { IsBackground = true }.Start();

            const int requestCount = 1000;

            var elapsed = Send(client, 1000);

            Thread.Sleep(3000);

            var elapsedAscending = elapsed.OrderBy(x => x).ToArray();
            Console.WriteLine("Count {0}", requestCount);
            Console.WriteLine("20% {0}", elapsedAscending[(int)(requestCount * .2)]);
            Console.WriteLine("50% {0}", elapsedAscending[(int)(requestCount * .5)]);
            Console.WriteLine("85% {0}", elapsedAscending[(int)(requestCount * .85)]);
            Console.WriteLine("95% {0}", elapsedAscending[(int)(requestCount * .95)]);
            Console.WriteLine("99% {0}", elapsedAscending[(int)(requestCount * .99)]);

            client.Dispose();
        }

        private static long[] Send(ThriftClient client, int requestCount)
        {
            var elapsed = Enumerable.Repeat(Int64.MaxValue, requestCount).ToArray();
            var stopwatch = new Stopwatch();

            Action<TProtocol> request = null;
            request = (input) =>
            {
                stopwatch.Restart();
                input.WriteString("Hello, world!");
                input.Transport.Flush();
            };

            Action<TProtocol, Exception, int> response = null;
            response = (output, exception, n) =>
            {
                if (exception != null)
                {
                    throw exception;
                }

                output.ReadString();

                if (n < requestCount)
                {
                    client.Send(i => request(i), (o, e) => response(o, e, n + 1));
                }

                elapsed[n - 1] = stopwatch.ElapsedMilliseconds;
            };

            client.Send((i) => request(i), (o, e) => response(o, e, 1));

            return elapsed;
        }
    }
}
