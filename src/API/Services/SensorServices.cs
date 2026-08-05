using System;
using API.Data;
using API.DTOs;
using API.Models;

namespace API.Services
{
    public class SensorServices : ISensorServices
    {
        private readonly AppDbContext _db;

        public SensorServices(AppDbContext db)
        {
            _db = db;
        }

        public async Task<bool> SaveSensorAsync(SensorDTORequests sensor)
        {
            try
            {
                if (sensor == null)
                {
                    throw new ArgumentNullException(nameof(sensor));
                }

                var newSensor = new Sensor
                {
                    Equipment = sensor.Equipment,
                    Current = sensor.Current,
                    Power = sensor.Power,
                    Location = sensor.Location,
                    Unit = sensor.Unit,
                    DataBaseTimestamp = DateTime.UtcNow,
                    Timestamp = sensor.Timestamp
                };

                _db.Sensors.Add(newSensor);
                await _db.SaveChangesAsync();
                return true;
            } catch (Exception ex)
            {
                // Log the exception (you can use a logging framework here)
                Console.WriteLine($"An error occurred while saving the sensor: {ex.Message}");
                return false;
            }

            
        }
    }
}