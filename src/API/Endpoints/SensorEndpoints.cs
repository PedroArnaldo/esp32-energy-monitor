using System.Text.Json;
using API.Models;
using API.Services;

namespace API.Endpoints;

public static class SensorEndpoints
{
    public static void MapSensorEndpoints(this IEndpointRouteBuilder routes)
    {
        var group = routes.MapGroup("/api/sensors").WithTags("Sensor");

        // group.MapGet("/", async (AppDbContext db) =>
        // {
        //     return await db.Sensors.ToListAsync();
        // })
        // .WithName("GetAllSensors")
        // .Produces<List<Sensor>>(StatusCodes.Status200OK);

        group.MapGet("/active", async () =>
        {
            var result = JsonSerializer.Serialize("{'active': true}");

            return Results.Ok(result);
        })
        .WithName("GetActiveSensors")
        .Produces(StatusCodes.Status200OK)
        .Produces(StatusCodes.Status404NotFound);

        group.MapPost("/save", async (Sensor sensor, ISensorServices sensorServices) =>
        {
           return await sensorServices.SaveSensorAsync(sensor);
        })
        .WithName("SaveSensor")
        .Produces(StatusCodes.Status204NoContent)
        .Produces(StatusCodes.Status404NotFound);
    }
}