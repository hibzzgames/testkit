// ----------------------------------------------------------------------------
// Author:      Sliptrixx (Hibnu Hishath)
// Date:        2024-06-10
//
// Description: TestKit is a simple testing framework used to perform unit 
//              testing on c++ code
// ----------------------------------------------------------------------------

#ifndef TESTKIT_H
#define TESTKIT_H

// ----------------------------------------------------------------------------
// Headers
// ----------------------------------------------------------------------------
#include <cassert>
#include <format>
#include <list>
#include <stack>
#include <source_location>
#include <vector>

// ----------------------------------------------------------------------------
// Constants
// ----------------------------------------------------------------------------
#define CHECK_MARK      "✓"
#define CROSS_MARK      "✘"
#define CIRCLE_SYM      "○"
#define ANSI_RESET      "\x1b[0m"
#define ANSI_GRAY       "\x1b[38;5;246m"
#define ANSI_GREEN      "\x1b[38;5;42m"
#define ANSI_RED        "\x1b[38;5;196m"
#define ANSI_DARK_GREEN "\x1b[38;5;28m"
#define ANSI_DARK_RED   "\x1b[38;5;160m"
#define ANSI_ITALIC     "\x1b[3m"

// ----------------------------------------------------------------------------
// Forward Declaration
// ----------------------------------------------------------------------------
namespace TestKit { enum class Outcome; }
namespace TestKit { struct Options; }
namespace TestKit { struct Node; }
namespace TestKit { struct Segment; }
namespace TestKit { struct SegmentScopeManager; }
namespace TestKit { struct Task; }

// ----------------------------------------------------------------------------
// TestKit Outcome Enum
// ----------------------------------------------------------------------------
enum class TestKit::Outcome {
    None,   // the test did not run
    Failed,
    Passed,
};

// ----------------------------------------------------------------------------
// TestKit Options struct
// ----------------------------------------------------------------------------
struct TestKit::Options
{
    int detailDepth; // How deep in the tree should the reporter continue reporting content in detail? Use -1 to show everything
};

// ----------------------------------------------------------------------------
// TestKit Report Generator functions
// ----------------------------------------------------------------------------
namespace TestKit::ReportGenerator 
{
    std::string Stringify( const Segment* segment, int depth );
    std::string Stringify( const Task* task, int depth );
};

// ----------------------------------------------------------------------------
// TestKit Node struct
// ----------------------------------------------------------------------------
struct TestKit::Node 
{
    virtual Outcome Check() const = 0;              // Check if the node is in a state to pass or fail
};

// ----------------------------------------------------------------------------
// TestKit Task struct
// ----------------------------------------------------------------------------
struct TestKit::Task : public TestKit::Node
{
    static Task Build( std::string name, std::source_location source );                 // A task with a given name that didn't run
    static Task Build( std::string name, std::source_location source, bool result );    // A task with a given with a result available

    friend std::string ReportGenerator::Stringify( const Task*, int );

    Outcome Check() const override;

private:
    std::string m_name;                 // a title given to this test 
    std::source_location m_source;      // the point in the codebase where this test was executed
    Outcome m_outcome = Outcome::None;  // the outcome of running this task
};

// ----------------------------------------------------------------------------
// TestKit Segment struct
// ----------------------------------------------------------------------------
struct TestKit::Segment : public TestKit::Node
{
    // Build a new task with the given name
    static Segment Build( std::string name );

    friend void Reset();
    friend std::string ReportGenerator::Stringify( const Segment*, int );

    Segment* AddSegment( Segment segment ); // Add the given segment as a sub-segment to this segment
    Task* AddTask( Task task );             // Add the given task under this segment
    void MarkFailed() { m_didFail = true; } // Mark this segment as failed blocking future tasks from running
    
    bool DidFail() const { return m_didFail; }  // Has this segment have a required task fail yet?

    Outcome Check() const override;
    
private:
    std::string m_name;                 // the title given to the task
    std::list< Segment > m_segments;  // a list of segments under this segment
    std::list< Task > m_tasks;        // a list of subtasks directly under this segment
    std::vector< Node* > m_nodes;       // ordered list of tasks and segments
    bool m_didFail = false;             // is this segment in a failed state?
};

// ----------------------------------------------------------------------------
// TestKit Segment Scope Manager struct
// ----------------------------------------------------------------------------
struct TestKit::SegmentScopeManager
{
    SegmentScopeManager( std::string name ); // pushes a new segment to the working stack
    ~SegmentScopeManager();                  // pops the last added segment from the working stack

    explicit operator bool();
};

// ----------------------------------------------------------------------------
// TestKit core functions and properties
// ----------------------------------------------------------------------------
namespace TestKit
{
    Segment __internal_root = Segment::Build( "" );                             // the main root segment hosting all subtasks and children segments
    std::stack< Segment* > __internal_segment_stack ( { &__internal_root } );   // the stack maintaining how the segments are stacked in scope
    
    Options __internal_curr_options = Options{ .detailDepth = -1 };

    void SetNewOptions( Options newOptions ) { __internal_curr_options = newOptions; }
    void Reset();
    std::string GenerateReport();
}

// ----------------------------------------------------------------------------
// TestKit Report Generator implementation
// ----------------------------------------------------------------------------
std::string TestKit::ReportGenerator::Stringify( const TestKit::Task* task, int depth )
{
    // ensure task is not a nullptr
    if( !task ) { return ""; }
    if( depth < 0 ) { return ""; }

    std::string out = std::string( depth * 2, ' ' ); // 2 spaces per depth
    
    Outcome outcome = task->Check();
    if( outcome == Outcome::Passed )
    {
        out += ANSI_GREEN CHECK_MARK;
    }
    else if( outcome == Outcome::None )
    {
        out += ANSI_GRAY CIRCLE_SYM;
    }
    else // Outcome::Failure
    {
        out += ANSI_RED CROSS_MARK;
    }

    out += " " + task->m_name;
    if( outcome == Outcome::Failed )
    {
        out += std::format( " ( at file: {}, line: {} )", task->m_source.file_name(), task->m_source.line() );
    }
    out += ANSI_RESET;
    return out;
}

std::string TestKit::ReportGenerator::Stringify( const TestKit::Segment* segment, int depth )
{
    // ensure segment isn't a nullptr
    if( !segment ) { return ""; }

    std::string out = depth < 0 ? "" : std::string( depth * 2, ' ' ); // 2 spaces per depth
    
    Outcome outcome = segment->Check();
    if( outcome == Outcome::None )
    {
        out += ANSI_GRAY;
    }
    out += segment->m_name;
    if( outcome != Outcome::None )
    {
        out += ":";
        if( outcome == Outcome::Passed )
        {
            out += ANSI_ITALIC ANSI_DARK_GREEN " [all tests passed]";
        }
        else if( outcome == Outcome::Failed )
        {
            out += ANSI_ITALIC ANSI_DARK_RED " [some tests failed]";
        }
        out += ANSI_RESET;
        if( depth < 0 ) { out = ""; } // depth is in the negative, ignore whatever was done for this depth and continue rendering the child

        if( depth < (uint16_t) __internal_curr_options.detailDepth || outcome == Outcome::Failed ) // respect the detail depth. However, failed nodes must be expanded regardless of depth to get more insights
        {
            for( auto node : segment->m_nodes )
            {
                if( Segment* subSegment = dynamic_cast< Segment* >( node ) )
                {
                    if( !out.ends_with( "\n" ) ) { out += "\n"; } // segment padding
                    out += "\n" + Stringify( subSegment, depth + 1 ) + "\n";
                }
                else if( Task* subTask = dynamic_cast< Task* >( node ) )
                {
                    out += "\n" + Stringify( subTask, depth + 1 );
                }
                else
                {
                    out += "AAAAGHHHH!!! ERROR... ERROR";
                }
            }
        }
    }
    out += ANSI_RESET;
    return out;
}

// ----------------------------------------------------------------------------
// TestKit Task implementation
// ----------------------------------------------------------------------------
TestKit::Task TestKit::Task::Build( std::string name, std::source_location source )
{
    TestKit::Task out;
    out.m_source = source;
    out.m_name = name;
    return out; 
}

TestKit::Task TestKit::Task::Build( std::string name, std::source_location source, bool result )
{
    TestKit::Task out = Build( name, source );
    out.m_outcome = result ? Outcome::Passed : Outcome::Failed;
    return out;
}

TestKit::Outcome TestKit::Task::Check() const
{
    return m_outcome;
}

// ----------------------------------------------------------------------------
// TestKit Segment implementation
// ----------------------------------------------------------------------------
TestKit::Segment TestKit::Segment::Build( std::string name )
{
    TestKit::Segment out;
    out.m_name = name;
    return out;
}

TestKit::Segment* TestKit::Segment::AddSegment( Segment segment )
{
    segment.m_didFail = m_didFail;
    m_segments.push_back( segment );
    m_nodes.push_back( &m_segments.back() );
    return &m_segments.back();
}

TestKit::Task* TestKit::Segment::AddTask( Task task )
{
    m_tasks.push_back( task );
    m_nodes.push_back( &m_tasks.back() );
    return &m_tasks.back();
}

TestKit::Outcome TestKit::Segment::Check() const
{
    // no nodes to run in this segment
    if( m_nodes.size() == 0 ) { return Outcome::None; }

    bool allPassed  = true;
    bool allAreNone = true;

    for( auto node : m_nodes )
    {
        Outcome outcome = node->Check();
        if( outcome == Outcome::Failed )
        {
            return Outcome::Failed;     // any node is failure? outcome is failure
        }

        if( outcome != Outcome::Passed ) { allPassed = false; } // some node failed to pass
        if( outcome != Outcome::None ) { allAreNone = false; }  // some node didn't run
    }

    if( allPassed )     { return Outcome::Passed; } // all nodes passed? outcome is passed
    if( allAreNone )    { return Outcome::None; }   // all nodes didn't run? outcome is none

    assert( !allPassed && !allAreNone );    // some passed and some didn't run? wtf??! weird edge case (assert for now)
    return Outcome::Failed;
}

// ----------------------------------------------------------------------------
// TestKit Segment Scope Manager implementation
// ----------------------------------------------------------------------------
TestKit::SegmentScopeManager::SegmentScopeManager( std::string name )
{
    Segment* top = ::TestKit::__internal_segment_stack.top();
    Segment* newSegment = top->AddSegment( Segment::Build( name ) );
    ::TestKit::__internal_segment_stack.push( newSegment );
}

TestKit::SegmentScopeManager::~SegmentScopeManager()
{
    assert( ::TestKit::__internal_segment_stack.size() > 1 );
    ::TestKit::__internal_segment_stack.pop();
}

TestKit::SegmentScopeManager::operator bool()
{
    return true;
}

// ----------------------------------------------------------------------------
// TestKit core function implementation
// ----------------------------------------------------------------------------
void TestKit::Reset()
{
    __internal_root.m_didFail = false;
    __internal_root.m_nodes.clear();
    __internal_root.m_segments.clear();
    __internal_root.m_tasks.clear();
    while( __internal_segment_stack.size() > 0 )
    {
        __internal_segment_stack.pop();
    }
    __internal_segment_stack.push( &__internal_root );
}

std::string TestKit::GenerateReport()
{
    std::string report = ReportGenerator::Stringify( &__internal_root, -1 );
    report = report.substr( report.find_first_not_of( "\n" ) );
    return report;
}

// ----------------------------------------------------------------------------
// Macros
// ----------------------------------------------------------------------------
#define __INTERNAL_TK_CAT( A, B ) A ## B
#define __INTERNAL_TK_RECAT( A, B ) __INTERNAL_TK_CAT( A, B )
#define __INTERNAL_TK_SELECT( NAME, NUM ) __INTERNAL_TK_CAT( NAME ## _, NUM )

#define __INTERNAL_TK_GET_COUNT( _1, _2, COUNT, ... ) COUNT
#define __INTERNAL_TK_VA_SIZE( ... ) __INTERNAL_TK_GET_COUNT( __VA_ARGS__, 2, 1 )

#define __INTERNAL_TK_VA_SELECT( NAME, ... ) __INTERNAL_TK_SELECT( NAME, __INTERNAL_TK_VA_SIZE(__VA_ARGS__) )(__VA_ARGS__)

#define __INTERNAL_UNIQUE_NAME( NAME ) __INTERNAL_TK_RECAT( NAME, __COUNTER__ )

#define __INTERNAL_TK_REQUIRE_2( msg, condition )                                                   \
{                                                                                                   \
    auto top = ::TestKit::__internal_segment_stack.top();                                           \
    if( top->DidFail() )                                                                            \
    {                                                                                               \
        top->AddTask( ::TestKit::Task::Build( msg, std::source_location::current() ) );             \
    }                                                                                               \
    else                                                                                            \
    {                                                                                               \
        bool c = condition; /* caching to prevent re-evaluation */                                  \
        if( !c ) { top->MarkFailed(); }                                                             \
        top->AddTask( ::TestKit::Task::Build( msg, std::source_location::current(), c ) );          \
    }                                                                                               \
}

#define __INTERNAL_TK_CHECK_2( msg, condition )                                                     \
{                                                                                                   \
    auto top = ::TestKit::__internal_segment_stack.top();                                           \
    if( top->DidFail() )                                                                            \
    {                                                                                               \
        top->AddTask( ::TestKit::Task::Build( msg, std::source_location::current() ) );             \
    }                                                                                               \
    else                                                                                            \
    {                                                                                               \
        top->AddTask( ::TestKit::Task::Build( msg, std::source_location::current(), condition ) );  \
    }                                                                                               \
}

#define __INTERNAL_TK_REQUIRE_1( condition ) __INTERNAL_TK_REQUIRE_2( #condition, condition )
#define __INTERNAL_TK_CHECK_1( condition ) __INTERNAL_TK_CHECK_2( #condition, condition )

#define SECTION( name ) if( ::TestKit::SegmentScopeManager __INTERNAL_UNIQUE_NAME( __testkit_segment_scope ) = ::TestKit::SegmentScopeManager( name ) )
#define REQUIRE( ... ) __INTERNAL_TK_VA_SELECT( __INTERNAL_TK_REQUIRE, __VA_ARGS__ )
#define CHECK( ... ) __INTERNAL_TK_VA_SELECT( __INTERNAL_TK_CHECK, __VA_ARGS__ )

#endif // TESTKIT_H
