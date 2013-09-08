using System;
using System.Diagnostics;
using System.Linq;
using System.Threading;

namespace Thrift.Client.Run
{
    internal static class Run
    {
        private static void Main()
        {
            const int count = 16;

            var client = new ThriftClient();

            new Thread(client.Run) { IsBackground = true }.Start();

            var elapsed = Enumerable.Repeat(Int64.MaxValue, count).ToArray();

            for (var i = 0; i < count; i++)
            {
                var stopwatch = Stopwatch.StartNew();
                var ii = i;
                client.Send(
                    input =>
                        {
                            input.WriteString("Hello, world!");
                            input.Transport.Flush();
                        },
                    (output, exception) =>
                        {
                            if (exception != null)
                            {
                                Console.WriteLine(exception.Message);
                                return;
                            }

                            var s = output.ReadString();
                            //Console.WriteLine(s);
                            elapsed[ii] = stopwatch.ElapsedMilliseconds;
                        });
            }

            Thread.Sleep(3000);

            var elapsedAscending = elapsed.OrderBy(x => x).ToArray();
            Console.WriteLine("Count {0}", count);
            Console.WriteLine("20% {0}", elapsedAscending[(int)(count * .2)]);
            Console.WriteLine("50% {0}", elapsedAscending[(int)(count * .5)]);
            Console.WriteLine("85% {0}", elapsedAscending[(int)(count * .85)]);
            Console.WriteLine("95% {0}", elapsedAscending[(int)(count * .95)]);
            Console.WriteLine("99% {0}", elapsedAscending[(int)(count * .99)]);

            client.Dispose();
        }
    }
}
