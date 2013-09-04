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
            const int count = 1;

            var client = new ThriftClient();

            //new Thread(client.Run) { IsBackground = true }.Start();

            var elapsed = new long[count];

            var totalStopwatch = Stopwatch.StartNew();

            for (var i = 0; i < count; i++)
            {
                var stopwatch = Stopwatch.StartNew();
                var local = i;
                client.Send(
                    requestTransport =>
                        {
                            var protocol = new TBinaryProtocol(requestTransport);
                            protocol.WriteI64(1);
                            protocol.WriteString("Hello, world!");
                            requestTransport.Flush();

                            elapsed[local] = stopwatch.ElapsedMilliseconds;
                        },
                    (responseTransport, exception) =>
                        {
                            if (exception != null)
                            {
                                Console.WriteLine(exception.Message);
                            }
                        });
            }

            client.Run();

            var totalElapsed = totalStopwatch.Elapsed;

            Thread.Sleep(3000);

            var elapsedAscending = elapsed.OrderBy(x => x).ToArray();
            Console.WriteLine("Count {0}", count);
            Console.WriteLine("20% {0}", elapsedAscending[(int)(count * .2)]);
            Console.WriteLine("50% {0}", elapsedAscending[(int)(count * .5)]);
            Console.WriteLine("85% {0}", elapsedAscending[(int)(count * .85)]);
            Console.WriteLine("95% {0}", elapsedAscending[(int)(count * .95)]);
            Console.WriteLine("99% {0}", elapsedAscending[(int)(count * .99)]);
            Console.WriteLine("Total elapsed {0}", totalElapsed);

            client.Dispose();
        }
    }
}
