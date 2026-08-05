using API.DTOs;

namespace API.Services
{
    public interface ISensorServices
    {
        //Task<List<Sensor>> GetAllSensorsAsync();
       // Task<Sensor> GetSensorByIdAsync(int id);
        Task<bool> SaveSensorAsync(SensorDTORequests sensor);
        //Task<bool> UpdateSensorAsync(int id, Sensor sensor);
    }
}