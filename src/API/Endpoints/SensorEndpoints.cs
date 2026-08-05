using System.Text.Json;
using API.DTOs;
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

        group.MapPost("/save", async (SensorDTORequests sensor, ISensorServices sensorServices) =>
        {
           try
           {
               var success = await sensorServices.SaveSensorAsync(sensor);
               if (success)
               {
                   return Results.NoContent();
               }
               else
               {
                   return Results.NotFound();
               }
           }
           catch (Exception ex)
           {
               // Log the exception (you can use a logging framework here)
               Console.WriteLine($"An error occurred while saving the sensor: {ex.Message}");
               return Results.Problem("An error occurred while saving the sensor.");
           }
        })
        .WithName("SaveSensor")
        .Produces(StatusCodes.Status204NoContent)
        .Produces(StatusCodes.Status404NotFound);
    }
}