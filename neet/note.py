import heapq

def dijkstra(graph, start):
    distances = {node: float('inf') for node in graph}
    distances[start] = 0
    pq = [(0, start)]  # (distance, node)
    
    while pq:
        curr_dist, curr_node = heapq.heappop(pq)
        
        if curr_dist > distances[curr_node]:
            continue
            
        for neighbor, weight in graph[curr_node]:
            distance = curr_dist + weight
            if distance < distances[neighbor]:
                distances[neighbor] = distance
                heapq.heappush(pq, (distance, neighbor))
    
    return distances



from collections import deque

def bfs(graph, start):
    visited = set()
    queue = deque([start])
    visited.add(start)
    
    while queue:
        node = queue.popleft()
        # Process node
        
        for neighbor in graph[node]:
            if neighbor not in visited:
                visited.add(neighbor)
                queue.append(neighbor)
    
    return visited




def merge_intervals(intervals):
    if not intervals:
        return []
    
    intervals.sort(key=lambda x: x[0])
    merged = [intervals[0]]
    
    for current in intervals[1:]:
        last = merged[-1]
        if current[0] <= last[1]:  # Overlap
            merged[-1] = (last[0], max(last[1], current[1]))
        else:
            merged.append(current)
    
    return merged





"""
TOP 3 ALGORITHMS FOR SPACEX STARLINK CONSTELLATION INTERVIEW
============================================================

These are the most critical algorithms for constellation problems:
1. Dijkstra's Algorithm - Routing through satellite mesh network
2. Interval Merging - Coverage window analysis
3. Greedy Scheduling - Handoff optimization

Author: SpaceX Interview Prep
"""

from typing import Dict, List, Tuple, Optional
import heapq
from collections import defaultdict


# ============================================================================
# 1. DIJKSTRA'S ALGORITHM - Shortest Path in Weighted Graph
# ============================================================================

def dijkstra(graph: Dict[int, List[Tuple[int, int]]], start: int) -> Dict[int, int]:
    """
    Find shortest path from start node to all other nodes.
    
    Args:
        graph: adjacency list {node: [(neighbor, weight), ...]}
               Example: {1: [(2, 10), (3, 5)], 2: [(4, 1)], ...}
        start: starting node
    
    Returns:
        Dictionary of {node: shortest_distance_from_start}
    
    Time Complexity: O((V + E) log V)
        - V vertices, E edges
        - Each vertex added/removed from heap: O(V log V)
        - Each edge explored once: O(E log V)
    
    Space Complexity: O(V)
    
    USE CASE: Find lowest-latency route through Starlink satellite mesh
    """
    # Initialize distances to infinity
    distances = defaultdict(lambda: float('inf'))
    distances[start] = 0
    
    # Min-heap: (distance, node)
    # Python's heapq is a min-heap by default
    priority_queue = [(0, start)]
    
    # Set to track processed nodes (optimization)
    visited = set()
    
    while priority_queue:
        current_distance, current_node = heapq.heappop(priority_queue)
        
        # Skip if already processed (important optimization!)
        if current_node in visited:
            continue
        
        visited.add(current_node)
        
        # Explore neighbors
        for neighbor, weight in graph.get(current_node, []):
            distance = current_distance + weight
            
            # Only update if we found a shorter path
            if distance < distances[neighbor]:
                distances[neighbor] = distance
                heapq.heappush(priority_queue, (distance, neighbor))
    
    return dict(distances)


def dijkstra_with_path(graph: Dict[int, List[Tuple[int, int]]], 
                       start: int, 
                       end: int) -> Tuple[int, List[int]]:
    """
    Find shortest path AND the actual path taken.
    
    Returns:
        (shortest_distance, path_as_list)
        Returns (float('inf'), []) if no path exists
    
    USE CASE: Show actual route data takes through satellites
    """
    distances = defaultdict(lambda: float('inf'))
    distances[start] = 0
    
    # Track previous node for path reconstruction
    previous = {}
    
    priority_queue = [(0, start)]
    visited = set()
    
    while priority_queue:
        current_distance, current_node = heapq.heappop(priority_queue)
        
        if current_node in visited:
            continue
        
        visited.add(current_node)
        
        # Early exit if we reached destination
        if current_node == end:
            break
        
        for neighbor, weight in graph.get(current_node, []):
            distance = current_distance + weight
            
            if distance < distances[neighbor]:
                distances[neighbor] = distance
                previous[neighbor] = current_node
                heapq.heappush(priority_queue, (distance, neighbor))
    
    # Reconstruct path
    if end not in previous and end != start:
        return (float('inf'), [])  # No path exists
    
    path = []
    current = end
    while current in previous:
        path.append(current)
        current = previous[current]
    path.append(start)
    path.reverse()
    
    return (distances[end], path)


# ============================================================================
# 2. INTERVAL MERGING - Coverage Window Analysis
# ============================================================================

def merge_intervals(intervals: List[Tuple[int, int]]) -> List[Tuple[int, int]]:
    """
    Merge overlapping intervals.
    
    Args:
        intervals: List of (start, end) tuples
                   Example: [(1, 5), (3, 8), (10, 15)]
    
    Returns:
        List of merged intervals: [(1, 8), (10, 15)]
    
    Time Complexity: O(n log n) - dominated by sorting
    Space Complexity: O(n) - for output
    
    USE CASE: Determine continuous coverage periods from multiple satellites
    """
    if not intervals:
        return []
    
    # Sort by start time
    intervals.sort(key=lambda x: x[0])
    
    merged = [intervals[0]]
    
    for start, end in intervals[1:]:
        last_start, last_end = merged[-1]
        
        if start <= last_end:  # Overlapping or touching
            # Merge by extending the end time
            merged[-1] = (last_start, max(last_end, end))
        else:
            # No overlap, add as new interval
            merged.append((start, end))
    
    return merged


def find_coverage_gaps(intervals: List[Tuple[int, int]], 
                       required_start: int, 
                       required_end: int) -> List[Tuple[int, int]]:
    """
    Find gaps in coverage within required time window.
    
    Args:
        intervals: List of coverage windows [(start, end), ...]
        required_start: When coverage should begin
        required_end: When coverage should end
    
    Returns:
        List of gaps [(gap_start, gap_end), ...]
        Returns [] if continuous coverage exists
    
    USE CASE: Identify when user will lose satellite connection
    """
    if not intervals:
        return [(required_start, required_end)]
    
    # Merge overlapping intervals first
    merged = merge_intervals(intervals)
    
    gaps = []
    current_position = required_start
    
    for start, end in merged:
        # Gap before this interval?
        if start > current_position:
            gaps.append((current_position, start))
        
        # Move position to end of this interval
        current_position = max(current_position, end)
    
    # Gap at the end?
    if current_position < required_end:
        gaps.append((current_position, required_end))
    
    return gaps


def has_continuous_coverage(intervals: List[Tuple[int, int]], 
                           start_time: int, 
                           end_time: int) -> bool:
    """
    Check if intervals provide continuous coverage for time range.
    
    Returns:
        True if continuous coverage, False otherwise
    
    USE CASE: Verify if satellite constellation can serve user continuously
    """
    gaps = find_coverage_gaps(intervals, start_time, end_time)
    return len(gaps) == 0


# ============================================================================
# 3. GREEDY SCHEDULING - Handoff Optimization
# ============================================================================

def min_handoffs_schedule(intervals: List[Tuple[int, int, str]]) -> List[str]:
    """
    Minimize number of handoffs while maintaining continuous coverage.
    At each decision point, choose satellite that stays visible longest.
    
    Args:
        intervals: List of (start, end, satellite_id)
                   Example: [(0, 5, 'SAT_A'), (3, 10, 'SAT_B'), ...]
    
    Returns:
        List of satellite IDs in order of use
        Returns [] if continuous coverage impossible
    
    Time Complexity: O(n log n)
    Space Complexity: O(n)
    
    USE CASE: Minimize handoffs for smoother user experience
    """
    if not intervals:
        return []
    
    # Sort by start time
    intervals.sort(key=lambda x: x[0])
    
    schedule = []
    current_end = 0
    i = 0
    
    while i < len(intervals):
        # Find all satellites available at current_end
        available = []
        
        while i < len(intervals) and intervals[i][0] <= current_end:
            start, end, sat_id = intervals[i]
            if end > current_end:  # Only consider if extends coverage
                available.append((end, sat_id))
            i += 1
        
        if not available:
            # No satellite available - coverage gap!
            return []  # Cannot maintain continuous coverage
        
        # Greedy choice: pick satellite that ends latest
        available.sort(reverse=True)  # Sort by end time descending
        next_end, next_sat = available[0]
        
        schedule.append(next_sat)
        current_end = next_end
        
        # Check if we've covered everything
        if current_end >= max(interval[1] for interval in intervals):
            break
    
    return schedule


def optimal_handoff_schedule(satellites: List[Dict]) -> List[Dict]:
    """
    Create handoff schedule with timing details.
    
    Args:
        satellites: List of dicts with 'id', 'start', 'end'
                    Example: [{'id': 'SAT_A', 'start': 0, 'end': 300}, ...]
    
    Returns:
        List of handoff events:
        [
            {'satellite': 'SAT_A', 'connect_time': 0, 'disconnect_time': 300},
            {'satellite': 'SAT_B', 'connect_time': 300, 'disconnect_time': 500},
            ...
        ]
    
    USE CASE: Generate actual handoff schedule for constellation control
    """
    if not satellites:
        return []
    
    # Convert to tuples for processing
    intervals = [(sat['start'], sat['end'], sat['id']) for sat in satellites]
    intervals.sort()
    
    schedule = []
    current_time = 0
    i = 0
    
    while i < len(intervals):
        # Find best satellite at current time
        best_sat = None
        best_end = current_time
        
        # Check all satellites that are available now
        j = i
        while j < len(intervals) and intervals[j][0] <= current_time:
            start, end, sat_id = intervals[j]
            if end > best_end:
                best_end = end
                best_sat = sat_id
            j += 1
        
        if best_sat is None:
            # Coverage gap - find next available satellite
            if i < len(intervals):
                # Jump to next satellite's start time
                gap_start = current_time
                gap_end = intervals[i][0]
                schedule.append({
                    'type': 'GAP',
                    'start': gap_start,
                    'end': gap_end,
                    'duration': gap_end - gap_start
                })
                current_time = intervals[i][0]
                continue
            else:
                break
        
        # Add this satellite to schedule
        connect_time = current_time
        disconnect_time = best_end
        
        schedule.append({
            'satellite': best_sat,
            'connect_time': connect_time,
            'disconnect_time': disconnect_time,
            'duration': disconnect_time - connect_time
        })
        
        current_time = best_end
        i = j
    
    return schedule


def max_concurrent_satellites(intervals: List[Tuple[int, int]]) -> int:
    """
    Find maximum number of satellites visible simultaneously.
    
    Args:
        intervals: List of (start, end) visibility windows
    
    Returns:
        Maximum number of overlapping intervals
    
    Time Complexity: O(n log n)
    
    USE CASE: Determine peak resource requirements
    """
    if not intervals:
        return 0
    
    # Create events: +1 for start, -1 for end
    events = []
    for start, end in intervals:
        events.append((start, 1))   # Satellite becomes visible
        events.append((end, -1))     # Satellite disappears
    
    # Sort events (start events before end events at same time)
    events.sort(key=lambda x: (x[0], -x[1]))
    
    max_concurrent = 0
    current_count = 0
    
    for time, delta in events:
        current_count += delta
        max_concurrent = max(max_concurrent, current_count)
    
    return max_concurrent


# ============================================================================
# EXAMPLE USAGE AND TESTS
# ============================================================================

if __name__ == "__main__":
    print("=" * 80)
    print("SPACEX STARLINK CONSTELLATION ALGORITHMS")
    print("=" * 80)
    
    # ========================================================================
    # Example 1: Dijkstra's - Satellite Mesh Routing
    # ========================================================================
    print("\n1. DIJKSTRA'S ALGORITHM - Satellite Mesh Network")
    print("-" * 80)
    
    # Satellite mesh network with inter-satellite links (ISLs)
    # Numbers are latencies in milliseconds
    satellite_mesh = {
        1: [(2, 10), (3, 15)],      # SAT1 -> SAT2 (10ms), SAT1 -> SAT3 (15ms)
        2: [(1, 10), (4, 20), (5, 8)],
        3: [(1, 15), (5, 12)],
        4: [(2, 20), (6, 5)],
        5: [(2, 8), (3, 12), (6, 10)],
        6: [(4, 5), (5, 10)]
    }
    
    print("Satellite mesh network (latencies in ms):")
    print("    1 --10-- 2 --8-- 5")
    print("    |        |       |")
    print("   15       20      10")
    print("    |        |       |")
    print("    3 --12-- 5 --10- 6")
    print("                     |")
    print("             4 --5---+")
    print()
    
    # Find shortest paths from SAT1
    distances = dijkstra(satellite_mesh, 1)
    print("Shortest latencies from SAT1 to all satellites:")
    for sat, latency in sorted(distances.items()):
        print(f"  SAT{sat}: {latency}ms")
    
    # Find specific path
    distance, path = dijkstra_with_path(satellite_mesh, 1, 6)
    print(f"\nBest route from SAT1 to SAT6:")
    print(f"  Path: {' -> '.join(f'SAT{s}' for s in path)}")
    print(f"  Total latency: {distance}ms")
    
    # ========================================================================
    # Example 2: Interval Merging - Coverage Analysis
    # ========================================================================
    print("\n\n2. INTERVAL MERGING - Coverage Window Analysis")
    print("-" * 80)
    
    # Satellite visibility windows (in seconds)
    visibility_windows = [
        (0, 300),      # SAT_A: 0-5 minutes
        (240, 420),    # SAT_B: 4-7 minutes (overlaps with SAT_A)
        (400, 600),    # SAT_C: 6.5-10 minutes
        (700, 900),    # SAT_D: 11.5-15 minutes (gap before this!)
    ]
    
    print("Raw visibility windows:")
    for i, (start, end) in enumerate(visibility_windows):
        print(f"  SAT_{chr(65+i)}: {start}s - {end}s ({(end-start)/60:.1f} min)")
    
    # Merge overlapping windows
    merged = merge_intervals(visibility_windows)
    print("\nMerged coverage periods:")
    for start, end in merged:
        print(f"  {start}s - {end}s ({(end-start)/60:.1f} min)")
    
    # Find gaps
    gaps = find_coverage_gaps(visibility_windows, 0, 900)
    print("\nCoverage gaps:")
    if gaps:
        for start, end in gaps:
            print(f"  GAP: {start}s - {end}s ({(end-start)/60:.1f} min)")
    else:
        print("  No gaps - continuous coverage!")
    
    # Check continuous coverage
    has_coverage = has_continuous_coverage(visibility_windows, 0, 600)
    print(f"\nContinuous coverage 0-600s: {has_coverage}")
    
    has_coverage = has_continuous_coverage(visibility_windows, 0, 900)
    print(f"Continuous coverage 0-900s: {has_coverage}")
    
    # ========================================================================
    # Example 3: Greedy Scheduling - Handoff Optimization
    # ========================================================================
    print("\n\n3. GREEDY SCHEDULING - Handoff Optimization")
    print("-" * 80)
    
    # Satellites with visibility windows
    satellite_windows = [
        (0, 300, 'SAT_A'),
        (240, 420, 'SAT_B'),
        (400, 600, 'SAT_C'),
        (550, 750, 'SAT_D'),
        (700, 900, 'SAT_E'),
    ]
    
    print("Satellite visibility windows:")
    for start, end, sat_id in satellite_windows:
        print(f"  {sat_id}: {start}s - {end}s")
    
    # Minimum handoffs schedule
    schedule = min_handoffs_schedule(satellite_windows)
    print(f"\nOptimal handoff sequence: {' -> '.join(schedule)}")
    print(f"Number of handoffs: {len(schedule) - 1}")
    
    # Detailed schedule
    satellites_dict = [
        {'id': 'SAT_A', 'start': 0, 'end': 300},
        {'id': 'SAT_B', 'start': 240, 'end': 420},
        {'id': 'SAT_C', 'start': 400, 'end': 600},
        {'id': 'SAT_D', 'start': 550, 'end': 750},
        {'id': 'SAT_E', 'start': 700, 'end': 900},
    ]
    
    detailed_schedule = optimal_handoff_schedule(satellites_dict)
    print("\nDetailed handoff schedule:")
    for event in detailed_schedule:
        if event.get('type') == 'GAP':
            print(f"  ⚠️  GAP: {event['start']}s - {event['end']}s "
                  f"({event['duration']}s no coverage)")
        else:
            print(f"  📡 {event['satellite']}: {event['connect_time']}s - "
                  f"{event['disconnect_time']}s ({event['duration']}s)")
    
    # Maximum concurrent satellites
    intervals_only = [(start, end) for start, end, _ in satellite_windows]
    max_concurrent = max_concurrent_satellites(intervals_only)
    print(f"\nMaximum satellites visible simultaneously: {max_concurrent}")
    
    print("\n" + "=" * 80)
    print("END OF EXAMPLES")
    print("=" * 80)


# ============================================================================
# INTERVIEW TIPS
# ============================================================================
"""
WHEN TO USE EACH ALGORITHM:

DIJKSTRA'S:
-----------
✓ "Find shortest/fastest route through satellite network"
✓ "Minimize latency from satellite A to B"
✓ "Route data through constellation optimally"
✓ Any weighted graph shortest path problem

INTERVAL MERGING:
-----------------
✓ "Find continuous coverage periods"
✓ "Detect gaps in satellite visibility"
✓ "Merge overlapping time windows"
✓ "Check if coverage is continuous"

GREEDY SCHEDULING:
------------------
✓ "Minimize number of satellite handoffs"
✓ "Choose best satellite at each decision point"
✓ "Optimize handoff schedule"
✓ "Activity selection problems"


COMPLEXITY QUICK REFERENCE:
---------------------------
Dijkstra's:          O((V + E) log V) time, O(V) space
Interval Merging:    O(n log n) time, O(n) space
Greedy Scheduling:   O(n log n) time, O(n) space


COMMON INTERVIEW FOLLOW-UPS:
-----------------------------
Q: "What if satellites move (topology changes)?"
A: "Recompute routes periodically, or use dynamic routing protocols"

Q: "What if links fail?"
A: "Remove failed edges from graph, recalculate. Have backup routes."

Q: "How to handle capacity constraints?"
A: "Add capacity to edge weights, use network flow algorithms"

Q: "Optimize for throughput instead of latency?"
A: "Change edge weights to inverse of bandwidth, or use max-flow"

Q: "What if we want minimal handoffs AND minimal latency?"
A: "Multi-objective optimization - can use weighted combination"
"""
