# TestKit

![LICENSE](https://img.shields.io/badge/LICENSE-MIT-45a63c?style=for-the-badge) [![Twitter Follow](https://img.shields.io/badge/follow-%40hibzzgames-1DA1f2?logo=twitter&style=for-the-badge)](https://twitter.com/hibzzgames) [![Discord](https://img.shields.io/discord/695898694083412048?color=788bd9&label=DIscord&style=for-the-badge)](https://discord.gg/YXdJ8cZngB)

TestKit is a header-only unit testing framework written in C++. I wrote it because existing open-source frameworks had some minor inconveniences that I could not get over. So, this is tuned to my preferences. So, if you like it, please feel free to use it.

<br>

## How to write tests?
There are three important macros provided by the TestKit framework that make it easy to write the tests. 

The `CHECK` macro is used to quickly evaluate if a given condition is valid or not. Optionally a custom message can be provided.

```c++
CHECK( 1 + 2 == 3 );
CHECK( "Simple subtraction", 2 - 2 == 0 );
```

<br>

The `REQUIRE` macro is similar to `CHECK`, except it can block future tests (in the same section) from running if it fails.

```c++
REQUIRE( "There's at least 1 element in the vector", vec.size() > 0 );
CHECK( vec[0] == 1 ); // If vector is empty, this statement wouldn't run
```

<br>

A `SECTION` macro helps group a bunch of `CHECK`s, `REQUIRE`s, and children `SECTION`s together.

```c++
SECTION( "Calculator" )
{
    int a = 1;
    int b = 2;

    SECTION( "addition" )
 {
        CHECK( a + a == 2 );
        CHECK( a + b == 3 );
 }

    SECTION( "subtraction" )
 {
        int c = a - a; // scoped to this section
        
        CHECK( b - a == 1 );
        REQUIRE( c == 0 ); // if this 'require' test fails, the check below will not run
        CHECK( c - b == -2 );
 }

    CHECK( a / b == 0 );
}
```

<br>

## How to run and view results?

![A screenshot of the generated TestKit results](https://github.com/hibzzgames/TestKit/assets/37605842/afe98161-bb4d-4a85-8343-80f1a5500b6a)

None of these tests are automatically registered. You must call them from someplace in your codebase. The framework runs and stores the result in its backend. To get the generated results, use the following code segment:

```c++
std::string report = TestKit::GenerateReport();
std::cout << report;
```

<br>

The stored results can be cleared using the reset function.

```c++
TestKit::Reset();
```

<br>

## Options available
The `TestKit::Options` struct was created to store a bunch of options that control how the tests are run and the results are generated. New options can be set using the following function:

```c++
TestKit::Options options; // configure options
TestKit::SetNewOptions( options );
```

<br>

**Detail Depth:**
Currently, TestKit has only one option, `detailDepth`, which controls the depth of detailed reports. Once the specified depth is reached, only success or non-execution of a section is reported without showing additional details, except for failures.

<br>

## Have a question or want to contribute?
If you have any questions or want to contribute, please open an issue, create a pull request, or start a conversation on the GitHub discussion board.

The [community discord](https://discord.gg/YXdJ8cZngB) is available to chat about the tools we make. Feel free to join it!
