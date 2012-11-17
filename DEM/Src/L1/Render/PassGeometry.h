// Renders geometry batches, instanced when possible
// Uses sorting, lights. Batches are designed to minimize shader state switches.
// Input:
// Geometry, lights (if lighting is enabled)
// Output:
// Mid-RT or frame RT