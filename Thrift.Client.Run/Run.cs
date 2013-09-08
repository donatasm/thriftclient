﻿using System;
using System.Diagnostics;
using System.Linq;
using System.Threading;

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
                    input =>
                        {
                            input.WriteI32(19);
                            input.Transport.Flush();

                            elapsed[local] = stopwatch.ElapsedMilliseconds;
                        },
                    (output, exception) =>
                        {
                            if (exception != null)
                            {
                                Console.WriteLine(exception.Message);
                                return;
                            }

                            for (var j = 0; j < 10000; j++)
                            {
                                Console.WriteLine(output.ReadI32());
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
