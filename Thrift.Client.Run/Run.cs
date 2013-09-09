using System;
using System.Diagnostics;
using System.Linq;
using System.Threading;
using Thrift.Protocol;

namespace Thrift.Client.Run
{
    internal static class Run
    {
        private const int ConnectionCount = 256;
        private const int RequestCount = 1000;
        private const int TotalRequests = ConnectionCount * RequestCount;
        private const long Undefined = Int64.MaxValue;
        private static readonly ManualResetEventSlim AllDone = new ManualResetEventSlim();

        private static long _counter;

        private static void Main()
        {
            var client = new ThriftClient();
            new Thread(client.Run) { IsBackground = true }.Start();

            _counter = 0;
            var elapsed = new long[ConnectionCount][];

            for (var i = 0; i < ConnectionCount; i++)
            {
                elapsed[i] = Send(client, RequestCount);
            }

            var stopwatch = Stopwatch.StartNew();
            AllDone.Wait();
            Console.WriteLine("Total time taken: {0}", stopwatch.Elapsed);

            client.Dispose();
            AllDone.Dispose();

            var aggregateElapsed = elapsed.SelectMany(e => e).OrderBy(x => x).ToArray();
            Console.WriteLine("Total requests: {0}", _counter);
            Console.WriteLine("20% {0}", aggregateElapsed[(int)(TotalRequests * .2)]);
            Console.WriteLine("50% {0}", aggregateElapsed[(int)(TotalRequests * .5)]);
            Console.WriteLine("85% {0}", aggregateElapsed[(int)(TotalRequests * .85)]);
            Console.WriteLine("95% {0}", aggregateElapsed[(int)(TotalRequests * .95)]);
            Console.WriteLine("99% {0}", aggregateElapsed[(int)(TotalRequests * .99)]);
        }

        private static long[] Send(ThriftClient client, int requestCount)
        {
            var elapsed = Enumerable.Repeat(Undefined, requestCount).ToArray();

            Action<TProtocol> request = null;
            request = (input) =>
            {
                input.WriteString("Hello, world!");
                input.Transport.Flush();
            };

            Action<TProtocol, Exception, State> response = null;
            response = (output, exception, state) =>
            {
                elapsed[state.N - 1] = state.Stopwatch.ElapsedMilliseconds;

                _counter++;

                if (exception != null)
                {
                    throw exception;
                }

                output.ReadString();

                if (state.N < requestCount)
                {
                    state.Stopwatch.Restart();
                    client.Send(i => request(i), (o, e) => response(o, e, new State(state.N + 1, state.Stopwatch)));
                }

                if (_counter == TotalRequests)
                {
                    AllDone.Set();
                }
            };

            var startState = new State(1, Stopwatch.StartNew());
            client.Send((i) => request(i), (o, e) => response(o, e, startState));

            return elapsed;
        }

        private struct State
        {
            private readonly int _n;
            private readonly Stopwatch _stopwatch;

            public State(int n, Stopwatch stopwatch)
            {
                _n = n;
                _stopwatch = stopwatch;
            }

            public int N
            {
                get { return _n; }
            }

            public Stopwatch Stopwatch
            {
                get { return _stopwatch; }
            }
        }
    }
}
